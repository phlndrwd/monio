/******************************************************************************
* MONIO - Met Office NetCDF Input Output                                      *
*                                                                             *
* (C) Crown Copyright 2023, Met Office. All rights reserved.                  *
*                                                                             *
* This software is licensed under the terms of the 3-Clause BSD License       *
* which can be obtained from https://opensource.org/license/bsd-3-clause/.    *
******************************************************************************/
#include "Data.h"

#include <stdexcept>
#include <vector>

#include "oops/util/Logger.h"

#include "Constants.h"
#include "DataContainerDouble.h"
#include "DataContainerFloat.h"
#include "DataContainerInt.h"
#include "Monio.h"
#include "Utils.h"

namespace {
template<typename T> bool compareData(std::vector<T>& lhsVec, std::vector<T>& rhsVec) {
  if (lhsVec.size() == rhsVec.size()) {
    for (auto lhsIt = lhsVec.begin(), rhsIt = rhsVec.begin();
         lhsIt != lhsVec.end(); ++lhsIt , ++rhsIt) {
      if (*lhsIt != *rhsIt)
        return false;
    }
    return true;
  }
  return false;
}
}  // anonymous namespace

monio::Data::Data() {
  oops::Log::trace() << "Data::Data()" << std::endl;
}

bool monio::operator==(const monio::Data& lhs, const monio::Data& rhs) {
  if (lhs.dataContainers_.size() == rhs.dataContainers_.size()) {
    for (auto lhsIt = lhs.dataContainers_.begin(), rhsIt = rhs.dataContainers_.begin();
         lhsIt != lhs.dataContainers_.end(); ++lhsIt , ++rhsIt) {
      std::shared_ptr<DataContainerBase> lhsDataContainer = lhsIt->second;
      std::shared_ptr<DataContainerBase> rhsDataContainer = rhsIt->second;

      int lhsDataType = lhsDataContainer->getType();
      int rhsDataType = rhsDataContainer->getType();

      std::string lhsName = lhsDataContainer->getName();
      std::string rhsName = rhsDataContainer->getName();

      if (lhsDataType == rhsDataType && lhsName == rhsName) {
        switch (lhsDataType) {
          case consts::eDataTypes::eDouble: {
            std::shared_ptr<DataContainerDouble> lhsDataContainerDouble =
              std::static_pointer_cast<DataContainerDouble>(lhsDataContainer);
            std::shared_ptr<DataContainerDouble> rhsDataContainerDouble =
              std::static_pointer_cast<DataContainerDouble>(rhsDataContainer);

            if (compareData(lhsDataContainerDouble->getData(),
                          rhsDataContainerDouble->getData()) == false)
              return false;
            break;
          }
          case consts::eDataTypes::eFloat: {
            std::shared_ptr<DataContainerFloat> lhsDataContainerFloat =
              std::static_pointer_cast<DataContainerFloat>(lhsDataContainer);
            std::shared_ptr<DataContainerFloat> rhsDataContainerFloat =
              std::static_pointer_cast<DataContainerFloat>(rhsDataContainer);

            if (compareData(lhsDataContainerFloat->getData(),
                          rhsDataContainerFloat->getData()) == false)
              return false;
            break;
          }
          case consts::eDataTypes::eInt: {
            std::shared_ptr<DataContainerInt> lhsDataContainerInt =
              std::static_pointer_cast<DataContainerInt>(lhsDataContainer);
            std::shared_ptr<DataContainerInt> rhsDataContainerInt =
              std::static_pointer_cast<DataContainerInt>(rhsDataContainer);

            if (compareData(lhsDataContainerInt->getData(),
                          rhsDataContainerInt->getData()) == false)
              return false;
            break;
          }
          default:
            return false;
        }
      } else {
        return false;
      }
    }
  } else {
    return false;
  }
  return true;
}

void monio::Data::addContainer(std::shared_ptr<DataContainerBase> container) {
  oops::Log::trace() << "Data::addContainer()" << std::endl;
  const std::string& name = container->getName();
  auto it = dataContainers_.find(name);
  if (it == dataContainers_.end()) {
    dataContainers_.insert({name, container});
  }
}

void monio::Data::deleteContainer(const std::string& name) {
  oops::Log::trace() << "Data::deleteContainer()" << std::endl;
  auto it = dataContainers_.find(name);
  if (it != dataContainers_.end()) {
    dataContainers_.erase(name);
  }  // Non-existant container is a legitimate use-case.
}

void monio::Data::removeAllButTheseContainers(const std::vector<std::string>& names) {
  oops::Log::trace() << "Data::removeAllButTheseContainers()" << std::endl;
  std::vector<std::string> containerKeys = utils::extractKeys(dataContainers_);
  for (const std::string& containerKey : containerKeys) {
    if (utils::findInVector(names, containerKey) == false) {
      deleteContainer(containerKey);
    }
  }
}

bool monio::Data::isContainerPresent(const std::string& name) const {
  oops::Log::trace() << "Data::isContainerPresent()" << std::endl;
  auto it = dataContainers_.find(name);
  if (it != dataContainers_.end()) {
    return true;
  } else {
    return false;
  }
}

std::shared_ptr<monio::DataContainerBase>
                monio::Data::getContainer(const std::string& name) const {
  oops::Log::trace() << "Data::getContainer()" << std::endl;
  auto it = dataContainers_.find(name);
  if (it != dataContainers_.end()) {
    return it->second;
  } else {
    Monio::get().closeFiles();
    utils::throwException("DataContainer named \"" + name + "\" was not found.");
  }
}

std::map<std::string, std::shared_ptr<monio::DataContainerBase>>&
                                      monio::Data::getContainers() {
  oops::Log::trace() << "Data::getContainers()" << std::endl;
  return dataContainers_;
}

const std::map<std::string, std::shared_ptr<monio::DataContainerBase>>&
                                            monio::Data::getContainers() const {
  oops::Log::trace() << "Data::getContainers()" << std::endl;
  return dataContainers_;
}

std::vector<std::string> monio::Data::getDataContainerNames() const {
  oops::Log::trace() << "Data::getDataContainerNames()" << std::endl;
  return utils::extractKeys(dataContainers_);
}

void monio::Data::clear() {
  oops::Log::trace() << "Data::clear()" << std::endl;
  dataContainers_.clear();
}
