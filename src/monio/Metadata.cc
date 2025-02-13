/******************************************************************************
* MONIO - Met Office NetCDF Input Output                                      *
*                                                                             *
* (C) Crown Copyright 2023, Met Office. All rights reserved.                  *
*                                                                             *
* This software is licensed under the terms of the 3-Clause BSD License       *
* which can be obtained from https://opensource.org/license/bsd-3-clause/.    *
******************************************************************************/
#include "Metadata.h"

#include <algorithm>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "oops/util/Logger.h"

#include "AttributeDouble.h"
#include "AttributeInt.h"
#include "AttributeString.h"
#include "Monio.h"
#include "Utils.h"

monio::Metadata::Metadata() {
  oops::Log::trace() << "Metadata::Metadata()" << std::endl;
}

bool monio::operator==(const monio::Metadata& lhs,
                       const monio::Metadata& rhs) {
  // Compare dimensions
  if (lhs.dimensions_.size() == rhs.dimensions_.size()) {
    for (auto lhsIt = lhs.dimensions_.begin(), rhsIt = rhs.dimensions_.begin();
         lhsIt != lhs.dimensions_.end(); ++lhsIt , ++rhsIt) {
      if (lhsIt->second != rhsIt->second)
        return false;
    }
  }
  // Compare variables
  if (lhs.variables_.size() == rhs.variables_.size()) {
    for (auto lhsIt = lhs.variables_.begin(), rhsIt = rhs.variables_.begin();
         lhsIt != lhs.variables_.end(); ++lhsIt , ++rhsIt) {
      std::shared_ptr<Variable> lhsVariable = lhsIt->second;
      std::shared_ptr<Variable> rhsVariable = rhsIt->second;

      std::string lhsName = lhsVariable->getName();
      std::string rhsName = rhsVariable->getName();

      int lhsDataType = lhsVariable->getType();
      int rhsDataType = rhsVariable->getType();

      int lhsTotSize = lhsVariable->getTotalSize();
      int rhsTotSize = rhsVariable->getTotalSize();

      std::vector<std::pair<std::string, size_t>>& lhsDimsVec = lhsVariable->getDimensionsMap();
      std::vector<std::pair<std::string, size_t>>& rhsDimsVec = rhsVariable->getDimensionsMap();

      if (lhsName == rhsName && lhsDataType == rhsDataType && lhsTotSize == rhsTotSize &&
          lhsDimsVec.size() == rhsDimsVec.size()) {
        // Compare dimensions vectors
        for (auto lhsIt = lhsDimsVec.begin(), rhsIt = rhsDimsVec.begin();
             lhsIt != lhsDimsVec.end(); ++lhsIt , ++rhsIt) {
          if (lhsIt->first != rhsIt->first || lhsIt->second != rhsIt->second)
            return false;
        }
        if (lhsVariable->getAttributes().size() == rhsVariable->getAttributes().size()) {
          for (auto lhsIt = lhsVariable->getAttributes().begin(),
               rhsIt = rhsVariable->getAttributes().begin();
               lhsIt != lhsVariable->getAttributes().end(); ++lhsIt , ++rhsIt) {
            std::shared_ptr<AttributeBase> lhsVarAttr = lhsIt->second;
            std::shared_ptr<AttributeBase> rhsVarAttr = rhsIt->second;

            int lhsVarAttrType = rhsVarAttr->getType();
            int rhsVarAttrType = lhsVarAttr->getType();

            std::string lhsVarAttrName = lhsVarAttr->getName();
            std::string rhsVarAttrName = rhsVarAttr->getName();

            if (lhsVarAttrType == rhsVarAttrType && lhsVarAttrName == rhsVarAttrName) {
              switch (lhsVarAttrType) {
                case consts::eDataTypes::eDouble: {
                 std::shared_ptr<monio::AttributeDouble> lhsVarAttrDbl =
                        std::dynamic_pointer_cast<monio::AttributeDouble>(lhsVarAttr);
                  std::shared_ptr<monio::AttributeDouble> rhsVarAttrDbl =
                        std::dynamic_pointer_cast<monio::AttributeDouble>(rhsVarAttr);
                  if (lhsVarAttrDbl->getValue() != rhsVarAttrDbl->getValue())
                    return false;
                  break;
                }
                case consts::eDataTypes::eInt: {
                 std::shared_ptr<monio::AttributeInt> lhsVarAttrInt =
                        std::dynamic_pointer_cast<monio::AttributeInt>(lhsVarAttr);
                  std::shared_ptr<monio::AttributeInt> rhsVarAttrInt =
                        std::dynamic_pointer_cast<monio::AttributeInt>(rhsVarAttr);
                  if (lhsVarAttrInt->getValue() != rhsVarAttrInt->getValue())
                    return false;
                  break;
                }
                case consts::eDataTypes::eString: {
                  std::shared_ptr<monio::AttributeString> lhsVarAttrStr =
                        std::dynamic_pointer_cast<monio::AttributeString>(lhsVarAttr);
                  std::shared_ptr<monio::AttributeString> rhsVarAttrStr =
                        std::dynamic_pointer_cast<monio::AttributeString>(rhsVarAttr);
                  if (lhsVarAttrStr->getValue() != rhsVarAttrStr->getValue())
                    return false;
                  break;
                }
                default:
                  return false;
              }
            } else {
              return false;
            }
          }  // end for( auto lhsIt = lhsVariable->getAttributes().begin() ...
        } else {
          return false;
        }
      } else {
        return false;
      }
    }  // end for (auto lhsIt = lhs.variables_.begin() ...
  } else {
    return false;
  }
  // There is no comparison of global attributes as these are subject to change and (should be)
  // inconsequential to system functioning.
  return true;
}

const bool monio::Metadata::isDimDefined(const std::string& dimName) const {
  oops::Log::trace() << "Metadata::isDimDefined()" << std::endl;
  auto it = dimensions_.find(dimName);
  if (it != dimensions_.end()) {
    return true;
  } else {
    return false;
  }
}

const int monio::Metadata::getDimension(const std::string& dimName) const {
  oops::Log::trace() << "Metadata::getDimension()" << std::endl;
  if (isDimDefined(dimName) == true) {
    return dimensions_.at(dimName);
  } else {
    Monio::get().closeFiles();
    utils::throwException("Metadata::getDimension()> dimension \"" + dimName + "\" not found...");
  }
}

std::string monio::Metadata::getDimensionName(const int dimValue) {
  for (auto it = dimensions_.begin(); it != dimensions_.end(); ++it) {
    if (it->second == dimValue) {
      return it->first;
    }
  }
  return std::string(consts::kNotFoundError);
}

const std::string monio::Metadata::getDimensionName(const int dimValue) const {
  for (auto it = dimensions_.begin(); it != dimensions_.end(); ++it) {
    if (it->second == dimValue) {
      return it->first;
    }
  }
  return std::string(consts::kNotFoundError);
}

std::shared_ptr<monio::Variable> monio::Metadata::getVariable(const std::string& varName) {
  oops::Log::trace() << "Metadata::getVariable()> " << varName << std::endl;
  auto it = variables_.find(varName);
  if (it != variables_.end()) {
    return variables_.at(varName);
  } else {
    Monio::get().closeFiles();
    utils::throwException("Metadata::getVariable()> variable \"" + varName + "\" not found...");
  }
}

const std::shared_ptr<monio::Variable>
      monio::Metadata::getVariable(const std::string& varName) const {
  oops::Log::trace() << "Metadata::getVariable()> " << varName << std::endl;
  auto it = variables_.find(varName);
  std::shared_ptr<monio::Variable> variable;
  if (it != variables_.end()) {
    return variables_.at(varName);
  } else {
    Monio::get().closeFiles();
    utils::throwException("Metadata::getVariable()> variable \"" + varName + "\" not found...");
  }
}

std::vector<std::shared_ptr<monio::Variable>>
      monio::Metadata::getVariables(const std::vector<std::string>& varNames) {
  oops::Log::trace() << "Metadata::getVariables()> " << std::endl;
  std::vector<std::shared_ptr<monio::Variable>> variables;
  for (const auto& varName : varNames) {
    variables.push_back(getVariable(varName));
  }
  return variables;
}

const std::vector<std::shared_ptr<monio::Variable>>
      monio::Metadata::getVariables(const std::vector<std::string>& varNames) const {
  oops::Log::trace() << "Metadata::getVariables()> " << std::endl;
  std::vector<std::shared_ptr<monio::Variable>> variables;
  for (const auto& varName : varNames) {
    variables.push_back(getVariable(varName));
  }
  return variables;
}

std::vector<std::string> monio::Metadata::getVarStrAttrs(const std::string& attrName) {
  oops::Log::trace() << "Metadata::getVarStrAttrs()" << std::endl;
  std::vector<std::string> varNames = getVariableNames();
  return getVarStrAttrs(varNames, attrName);
}

const std::vector<std::string> monio::Metadata::getVarStrAttrs(
                                                  const std::vector<std::string>& varNames,
                                                  const std::string& attrName) const {
  oops::Log::trace() << "Metadata::getVarStrAttrs()" << std::endl;
  std::vector<std::shared_ptr<monio::Variable>> variables = getVariables(varNames);
  std::vector<std::string> varStrAttrs;
  for (const auto& var : variables) {
      std::string attr = var->getStrAttr(attrName);
      varStrAttrs.push_back(attr);
  }
  if (varNames.size() != varStrAttrs.size()) {
    Monio::get().closeFiles();
    utils::throwException("Metadata::getVarStrAttrs()> "
        "Unmatched number of variables and attributes...");
  }
  return varStrAttrs;
}

void monio::Metadata::addDimension(const std::string& dimName, const int value) {
  oops::Log::trace() << "Metadata::addDimension()" << std::endl;
  auto it = dimensions_.find(dimName);
  if (it == dimensions_.end()) {
    dimensions_.insert({dimName, value});
  }
}


void monio::Metadata::addGlobalAttr(const std::string& attrName,
                                    std::shared_ptr<AttributeBase> attr) {
  oops::Log::trace() << "Metadata::addGlobalAttr()" << std::endl;
  auto it = globalAttrs_.find(attrName);
  if (it == globalAttrs_.end()) {
    globalAttrs_.insert({attrName, attr});
  }
}

void monio::Metadata::addVariable(const std::string& varName,
                                  std::shared_ptr<Variable> var) {
  oops::Log::trace() << "Metadata::addVariable()" << std::endl;
  auto it = variables_.find(varName);
  if (it == variables_.end()) {
    variables_.insert({varName, var});
  }
}

std::vector<std::string> monio::Metadata::getDimensionNames() {
  oops::Log::trace() << "Metadata::getDimensionNames()" << std::endl;
  return utils::extractKeys(dimensions_);
}

std::vector<std::string> monio::Metadata::getVariableNames() {
  oops::Log::trace() << "Metadata::getVariableNames()" << std::endl;
  return utils::extractKeys(variables_);
}

std::vector<std::string> monio::Metadata::findVariableNames(const std::string& searchTerm) {
  std::vector<std::string> variableKeys = utils::extractKeys(variables_);
  std::vector<std::string> variableNames;
  for (const auto& variableKey : variableKeys) {
    std::size_t pos = variableKey.find(searchTerm);
    if (pos != std::string::npos) {
      variableNames.push_back(variableKey);
    }
  }
  return variableNames;
}

std::vector<std::string> monio::Metadata::getGlobalAttrNames() {
  oops::Log::trace() << "Metadata::getGlobalAttrNames()" << std::endl;
  return utils::extractKeys(globalAttrs_);
}

std::map<std::string, int>& monio::Metadata::getDimensionsMap() {
  oops::Log::trace() << "Metadata::getDimensionsMap()" << std::endl;
  return dimensions_;
}

std::map<std::string, std::shared_ptr<monio::Variable>>&
                                      monio::Metadata::getVariablesMap() {
  oops::Log::trace() << "Metadata::getVariablesMap()" << std::endl;
  return variables_;
}

std::map<std::string, std::shared_ptr<monio::AttributeBase>>&
                                      monio::Metadata::getGlobalAttrsMap() {
  oops::Log::trace() << "Metadata::getGlobalAttrsMap()" << std::endl;
  return globalAttrs_;
}

const std::map<std::string, int>& monio::Metadata::getDimensionsMap() const {
  oops::Log::trace() << "Metadata::getDimensionsMap()" << std::endl;
  return dimensions_;
}

const std::map<std::string, std::shared_ptr<monio::Variable>>&
                                            monio::Metadata::getVariablesMap() const {
  oops::Log::trace() << "Metadata::getVariablesMap()" << std::endl;
  return variables_;
}

const std::map<std::string, std::shared_ptr<monio::AttributeBase>>&
                                            monio::Metadata::getGlobalAttrsMap() const {
  oops::Log::trace() << "Metadata::getGlobalAttrsMap()" << std::endl;
  return globalAttrs_;
}

int monio::Metadata::getVariableConvention() {
  int variableConvention = consts::eLfricConvention;  // Defaults to LFRic Convention
  for (const auto& globalAttr : globalAttrs_) {
    if (globalAttr.first == consts::kVariableConventionName) {
      std::shared_ptr<monio::AttributeString> dataFormatAttr =
            std::dynamic_pointer_cast<monio::AttributeString>(globalAttr.second);
      std::string dataFormatValue = dataFormatAttr->getValue();
      int pos = utils::findPosInVector(consts::kNamingConventions, dataFormatValue);
      if (pos != -1) {  // Condition preserves use of consts::eNotDefined == 2 when not found
        variableConvention = pos;
      }
    }
  }
  return variableConvention;
}

void monio::Metadata::removeAllButTheseVariables(
    const std::vector<std::string>& varNames) {
  oops::Log::trace() << "Metadata::removeAllButTheseVariables()" << std::endl;
  std::vector<std::string> variableKeys = utils::extractKeys(variables_);
  for (const std::string& variableKey : variableKeys) {
    if (utils::findInVector(varNames, variableKey) == false) {
      deleteVariable(variableKey);
    }
  }
}

void monio::Metadata::deleteDimension(const std::string& dimName) {
  oops::Log::trace() << "Metadata::deleteDimension()" << std::endl;
  auto itDim = dimensions_.find(dimName);
  if (itDim != dimensions_.end()) {
    dimensions_.erase(dimName);
  }  // Non-existant dimension is a legitimate use-case.
  for (const auto& varPair : variables_) {
    varPair.second->deleteDimension(dimName);
  }
}

void monio::Metadata::deleteVariable(const std::string& varName) {
  oops::Log::trace() << "Metadata::deleteVariable()" << std::endl;
  auto it = variables_.find(varName);
  if (it != variables_.end()) {
    variables_.erase(varName);
  } else {
    Monio::get().closeFiles();
    utils::throwException("Metadata::deleteVariable()> Variable \"" + varName + "\" not found...");
  }
}

void monio::Metadata::clear() {
  oops::Log::trace() << "Metadata::clear()" << std::endl;
  // dimensions_.clear();  // Dimensions are required for correct writing of subsequent variables.
  variables_.clear();
  globalAttrs_.clear();
}

void monio::Metadata::clearGlobalAttributes() {
  oops::Log::trace() << "Metadata::clearGlobalAttributes()" << std::endl;
  globalAttrs_.clear();
}

void monio::Metadata::print() {
  oops::Log::debug() << "dimensions:" << std::endl;
  printMap(dimensions_);
  oops::Log::debug() << "variables:" << std::endl;
  printVariables();
  oops::Log::debug() << "attributes:" << std::endl;
  printGlobalAttrs();
}

void monio::Metadata::printVariables() {
  for (auto const& var : variables_) {
    std::shared_ptr<Variable> netCDFVar = var.second;
    oops::Log::debug() << consts::kTabSpace <<
                          consts::kDataTypeNames[netCDFVar->getType()] <<
                           " " << netCDFVar->getName();
    std::vector<std::string> varDims = netCDFVar->getDimensionNames();
    if (varDims.size() > 0) {
      oops::Log::debug() << "(";
      for (auto it = varDims.begin(); it != varDims.end() - 1; ++it) {
        oops::Log::debug() << *it << ", ";
      }
      oops::Log::debug() << *(varDims.end() - 1) << ")" << std::endl;
    } else {
      oops::Log::debug() << std::endl;
    }
    std::map<std::string, std::shared_ptr<AttributeBase>>& varAttrsMap =
                                                        netCDFVar->getAttributes();
    for (auto const& varAttrPair : varAttrsMap) {
      std::shared_ptr<AttributeBase> netCDFAttr = varAttrPair.second;

      oops::Log::debug() << consts::kTabSpace <<
                            consts::kTabSpace <<
                            netCDFVar->getName() << ":" <<
                            netCDFAttr->getName() << " = ";
      int dataType = netCDFAttr->getType();
      switch (dataType) {
        case consts::eDataTypes::eDouble: {
            std::shared_ptr<monio::AttributeDouble> netCDFAttrDbl =
                        std::dynamic_pointer_cast<monio::AttributeDouble>(netCDFAttr);
          oops::Log::debug() << netCDFAttrDbl->getValue() << std::endl;
          break;
        }
        case consts::eDataTypes::eInt: {
          std::shared_ptr<monio::AttributeInt> netCDFAttrInt =
                        std::dynamic_pointer_cast<monio::AttributeInt>(netCDFAttr);
          oops::Log::debug() << netCDFAttrInt->getValue() << " ;" << std::endl;
          break;
        }
        case consts::eDataTypes::eString: {
            std::shared_ptr<monio::AttributeString> netCDFAttrStr =
                        std::dynamic_pointer_cast<monio::AttributeString>(netCDFAttr);
          oops::Log::debug() << std::quoted(netCDFAttrStr->getValue()) << std::endl;
          break;
        }
        default: {
          Monio::get().closeFiles();
          utils::throwException("Metadata::printGlobalAttrs()> Data type not coded for...");
        }
      }
    }
  }
}

void monio::Metadata::printGlobalAttrs() {
  for (const auto& globAttrPair : globalAttrs_) {
    oops::Log::debug() << consts::kTabSpace << globAttrPair.first;
    std::shared_ptr<AttributeBase> globalAttr = globAttrPair.second;
    int type = globalAttr->getType();
    switch (type) {
      case consts::eDataTypes::eDouble: {
        std::shared_ptr<monio::AttributeDouble> globAttrDbl =
                        std::dynamic_pointer_cast<monio::AttributeDouble>(globalAttr);
        oops::Log::debug() << " = " << globAttrDbl->getValue() << " ;"  << std::endl;
        break;
      }
      case consts::eDataTypes::eInt: {
        std::shared_ptr<monio::AttributeInt> globalAttrInt =
                        std::dynamic_pointer_cast<monio::AttributeInt>(globalAttr);
        oops::Log::debug() << globalAttrInt->getValue() << " ;" << std::endl;
        break;
      }
      case consts::eDataTypes::eString: {
        std::shared_ptr<monio::AttributeString> globAttrStr =
                        std::dynamic_pointer_cast<monio::AttributeString>(globalAttr);
        oops::Log::debug() << " = " << std::quoted(globAttrStr->getValue()) << " ;"  << std::endl;
        break;
      }
      default: {
        Monio::get().closeFiles();
        utils::throwException("Metadata::printGlobalAttrs()> Data type not coded for...");
      }
    }
  }
}

template<typename T>
void monio::Metadata::printMap(const std::map<std::string, T>& map) {
  for (const auto& entry : map) {
    oops::Log::debug() << consts::kTabSpace << entry.first <<
                           " = " << entry.second << " ;" << std::endl;
  }
}

template void monio::Metadata::printMap<int>(const std::map<std::string, int>& map);
template void monio::Metadata::printMap<std::string>(const std::map<std::string, std::string>& map);

