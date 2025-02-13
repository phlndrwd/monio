#!/bin/bash
#
# (C) Crown Copyright 2024, the Met Office. All rights reserved.
#
# shellcheck disable=SC2317
# shellcheck source=/dev/null
set -euo pipefail

finally() {
    trap '' ERR
    trap '' EXIT
    if [[ -d "${WORKD:-}" ]]; then
        cd /
        rm -fr "${WORKD}"
    fi
    if [[ -d ${BASE:-} ]]; then
        cd /
        rm -fr "$BASE" 2>/dev/null || true
    fi
}

# -- HERE is /var/tmp/<REPONAME>/pr-<#> (cf ../.github/workflow/ci.yml)
HERE="$(cd "$(dirname "$0")" && pwd)"
THIS="$(basename "$0")"
NPROC=${NPROC:-$(nproc)}
WORKD="$(mktemp -d "${THIS}-XXXXXX" -t)"
BASE="${HERE%/*}"
TESTDIR="${BASE##*/}"
export OMPI_ALLOW_RUN_AS_ROOT=1
export OMPI_ALLOW_RUN_AS_ROOT_CONFIRM=1
export CI_TESTS=${CI_TESTS:-0}
CI_TESTS=${CI_TESTS//$'\r'}  # remove those pesky invisible EOL characters

trap finally ERR EXIT

cd "${WORKD}"

# -- Activate spack env if using JCSDA Docker container
if [[ -f /opt/spack-environment/activate.sh ]]; then
    source /opt/spack-environment/activate.sh
fi

# -- Enable OpenMPI over subscription -----------------------------------------
if command -v ompi_info &>/dev/null; then
    ompi_vn=$(ompi_info | awk '/Ident string:/ {print $3}')
    case $ompi_vn in
        4.*) export OMPI_MCA_rmaps_base_oversubscribe=1 ;;
        5.*) export PRTE_MCA_rmaps_default_mapping_policy=:oversubscribe ;;
    esac
fi

if command -v ninja &>/dev/null; then
    GENERATOR=Ninja;
else
    GENERATOR=Unix\ Makefiles;
fi

# -- Configure
cmake -B . -S "${HERE}" -G "${GENERATOR}" -DCMAKE_BUILD_TYPE=Release

# -- Build
cmake --build . -j "${NPROC}"

# -- Test
case "$CI_TESTS" in
    0) ctest --test-dir "${TESTDIR}" --output-on-failure -R 'coding_norms' ;;
    1) ctest --test-dir "${TESTDIR}" --output-on-failure ;;
    *) echo "** Error: CI_TESTS=$CI_TESTS is not a valid option **";
       exit 1 ;;
esac

exit
