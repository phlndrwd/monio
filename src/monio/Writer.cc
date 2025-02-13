/******************************************************************************
* MONIO - Met Office NetCDF Input Output                                      *
*                                                                             *
* (C) Crown Copyright 2023, Met Office. All rights reserved.                  *
*                                                                             *
* This software is licensed under the terms of the 3-Clause BSD License       *
* which can be obtained from https://opensource.org/license/bsd-3-clause/.    *
******************************************************************************/
#include "Writer.h"

#include <netcdf>
#include <map>
#include <stdexcept>

#include "Constants.h"
#include "DataContainerDouble.h"
#include "DataContainerFloat.h"
#include "DataContainerInt.h"
#include "Utils.h"

#include "oops/util/Logger.h"

monio::Writer::Writer(const eckit::mpi::Comm& mpiCommunicator,
                      const int mpiRankOwner,
                      const std::string& filePath) :
    mpiCommunicator_(mpiCommunicator),
    mpiRankOwner_(mpiRankOwner) {
  oops::Log::trace() << "Writer::Writer()" << std::endl;
  openFile(filePath);
}

monio::Writer::Writer(const eckit::mpi::Comm& mpiCommunicator,
                      const int mpiRankOwner) :
    mpiCommunicator_(mpiCommunicator),
    mpiRankOwner_(mpiRankOwner) {
  oops::Log::trace() << "Writer::Writer()" << std::endl;
}

void monio::Writer::openFile(const std::string& filePath) {
  oops::Log::trace() << "Writer::openFile() \"" << filePath << "\"..." << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    if (filePath.size() != 0) {
      try {
        file_ = std::make_unique<File>(filePath, netCDF::NcFile::replace);
      } catch (netCDF::exceptions::NcException& exception) {
        closeFile();
        utils::throwException("Writer::openFile()> An exception occurred while creating File...");
      }
    }
  }
}

void monio::Writer::closeFile() {
  oops::Log::trace() << "Writer::closeFile()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    if (isOpen() == true) {
      getFile().close();
      file_.reset();
    }
  }
}

bool monio::Writer::isOpen() {
  return file_ != nullptr;
}

void monio::Writer::writeMetadata(const Metadata& metadata) {
  oops::Log::trace() << "Writer::writeMetadata()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    getFile().writeMetadata(metadata);
  }
}

void monio::Writer::writeData(const FileData& fileData) {
  oops::Log::trace() << "Writer::writeVariablesData()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    const std::map<std::string, std::shared_ptr<DataContainerBase>>& dataContainerMap =
                                                                fileData.getData().getContainers();
    for (const auto& dataContainerPair : dataContainerMap) {
      std::string varName = dataContainerPair.first;
      fileData.getMetadata().getVariable(varName);  // Checks variable exists in metadata
      std::shared_ptr<DataContainerBase> dataContainer = dataContainerPair.second;
      int dataType = dataContainerPair.second->getType();
      switch (dataType) {
        case consts::eDataTypes::eDouble: {
          std::shared_ptr<DataContainerDouble> dataContainerDouble =
              std::static_pointer_cast<DataContainerDouble>(dataContainer);
          getFile().writeSingleDatum(varName, dataContainerDouble->getData());
          break;
        }
        case consts::eDataTypes::eFloat: {
          std::shared_ptr<DataContainerFloat> dataContainerFloat =
              std::static_pointer_cast<DataContainerFloat>(dataContainer);
          getFile().writeSingleDatum(varName, dataContainerFloat->getData());
          break;
        }
        case consts::eDataTypes::eInt: {
          std::shared_ptr<DataContainerInt> dataContainerInt =
              std::static_pointer_cast<DataContainerInt>(dataContainer);
          getFile().writeSingleDatum(varName, dataContainerInt->getData());
          break;
        }
        default: {
          closeFile();
          utils::throwException("Writer::writeVariablesData()> Data type not coded for...");
        }
      }
    }
  }
}

monio::File& monio::Writer::getFile() {
  oops::Log::trace() << "Writer::getFile()" << std::endl;
  if (isOpen() == false) {
    utils::throwException("Writer::getFile()> File has not been initialised...");
  }
  return *file_;
}
