&copy; Crown Copyright Met Office 2023

[![CI](https://github.com/MetOffice/monio/actions/workflows/ci.yml/badge.svg)](https://github.com/MetOffice/monio/actions/workflows/ci.yml)


# Met Office NetCDF Input Output (MONIO)

## Description

MONIO is not just an acronym. It is also so-named because it is _single_-threaded code (mon-; one, single), runs on a _single_ PE in a full MPI context, and the main class is accessible via a _singleton_ design pattern. It also happens that there is a lesser-known Star Wars character with the same name (https://starwars.fandom.com/wiki/Monio) :neutral_face:

MONIO has been written in C++17 to handle file I/O for the Met Office's (MO) JEDI (https://jointcenterforsatellitedataassimilation-jedi-docs.readthedocs-hosted.com/) Data Assimilation (DA) model interfaces, but it is also developed with a gesture toward general applicability. MONIO represents an extendible set of classes to make reading and writing Network Common Data Form (NetCDF) files more friendly than using the bare NetCDF interface provided by Unidata alone.

MONIO provides a `Monio` singleton class specifically for reading and writing state and increment, UGRID-format (http://ugrid-conventions.github.io/ugrid-conventions/) files generated by and to be consumed by the MO's "LFRic" model (https://www.metoffice.gov.uk/research/approach/modelling-systems/lfric). For this reason, it carries a few dependencies that are characteristic of any other JEDI-based repository.

## How to Install and Run

### Dependencies

MONIO is a CMake project. However, it is configured to make use of ECMWF's ECBuild (https://github.com/ecmwf/ecbuild). The `CMakeLists.txt` file will need to modified to avoid this if a dependency on ECBuild is to be avoided.

At its core, MONIO uses and depends upon Unidata's NetCDF-C++ (https://github.com/Unidata/netcdf-cxx4) library. This carries its own dependencies on Unidata's NetCDF-C (https://github.com/Unidata/netcdf-c) and the HDF Group's HDF5 (https://github.com/HDFGroup/hdf5) libraries, as well as some other supporting libraries.

Having been built for use with JEDI, MONIO also carries a dependency on ECMWF's Atlas (https://github.com/ecmwf/atlas) library. However, MONIO is written in an attempt to compartmentalise this dependency in the `AtlasReader` and `AtlasWriter` classes. This puts users in a position to drop these classes to construct a NetCDF I/O solution without Atlas.

MONIO makes frequent use of JCSDA's OOPS (https://github.com/JCSDA/oops) `oops::Log` tool and its `util::DateTime` representation. Some light modifications could avoid these, but without modification MONIO will need to be compiled with a compatible version of OOPS.

As it is intended to run within an operational, meteorological DA context, MONIO is written to function with Message Passing Interface (MPI) communicators. JEDI wraps these with C++ code provided by ECMWF's ECKit (https://github.com/ecmwf/eckit), and MONIO adopts this approach. For this reason MONIO also carries a depenency on ECKit. Which itself carries an underlying dependency on a suitable MPI implementation, e.g. Open MPI (https://www.open-mpi.org/software/).

### Compilation

MONIO is an ecbuild

## Issues

Any questions or issues can be reported to philip.underwood@metoffice.gov.uk.
