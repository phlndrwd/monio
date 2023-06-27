/*#############################################################################
# MONIO - Met Office NetCDF Input Output                                      #
#                                                                             #
# (C) Crown Copyright 2023 Met Office                                         #
#                                                                             #
# This software is licensed under the terms of the Apache Licence Version 2.0 #
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.        #
#############################################################################*/
#include "DataContainerBase.h"

monio::DataContainerBase::DataContainerBase(
    const std::string& name,
    const int type) :
  name_(name), type_(type) {}

const int monio::DataContainerBase::getType() const {
  return type_;
}
