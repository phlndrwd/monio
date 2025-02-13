﻿
/******************************************************************************
* MONIO - Met Office NetCDF Input Output                                      *
*                                                                             *
* (C) Crown Copyright 2023, Met Office. All rights reserved.                  *
*                                                                             *
* This software is licensed under the terms of the 3-Clause BSD License       *
* which can be obtained from https://opensource.org/license/bsd-3-clause/.    *
******************************************************************************/
#include "AtlasWriter.h"

#include "atlas/grid/Iterator.h"
#include "oops/util/Logger.h"

#include "AttributeString.h"
#include "DataContainerDouble.h"
#include "DataContainerFloat.h"
#include "DataContainerInt.h"
#include "Metadata.h"
#include "Monio.h"
#include "Utils.h"
#include "UtilsAtlas.h"
#include "Writer.h"

monio::AtlasWriter::AtlasWriter(const eckit::mpi::Comm& mpiCommunicator, const int mpiRankOwner):
    mpiCommunicator_(mpiCommunicator),
    mpiRankOwner_(mpiRankOwner) {
  oops::Log::trace() << "AtlasWriter::AtlasWriter()" << std::endl;
}

void monio::AtlasWriter::populateFileDataWithField(FileData& fileData,
                                                   atlas::Field& field,
                                             const consts::FieldMetadata& fieldMetadata,
                                             const std::string& writeName,
                                             const std::string& vertConfigName,
                                             const bool isLfricConvention) {
  oops::Log::trace() << "AtlasWriter::populateFileDataWithField()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    std::vector<size_t>& lfricAtlasMap = fileData.getLfricAtlasMap();
    // Create dimensions
    Metadata& metadata = fileData.getMetadata();
    atlas::Field writeField;
    if (isLfricConvention == true) {
      writeField = getWriteField(field, writeName, fieldMetadata.noFirstLevel);
    } else {
      writeField = field;
    }
    populateMetadataWithField(metadata, writeField, fieldMetadata, writeName, vertConfigName);
    populateDataWithField(fileData.getData(), writeField, lfricAtlasMap, writeName);
    addGlobalAttributes(metadata, isLfricConvention);
  }
}

void monio::AtlasWriter::populateFileDataWithField(FileData& fileData,
                                             const atlas::Field& field,
                                             const std::string& writeName) {
  oops::Log::trace() << "AtlasWriter::populateFileDataWithField()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    Metadata& metadata = fileData.getMetadata();
    Data& data = fileData.getData();
    // Create dimensions
    std::vector<atlas::idx_t> fieldShape = field.shape();
    if (field.metadata().get<bool>("global") == false) {
      fieldShape[0] = utilsatlas::getHorizontalSize(field);
    }
    for (auto& dimSize : fieldShape) {
      std::string dimName = metadata.getDimensionName(dimSize);
      if (dimName == consts::kNotFoundError) {
        dimName = "dim" + std::to_string(dimCount_);
        fileData.getMetadata().addDimension(dimName, dimSize);
        dimCount_++;
      }
    }
    // Create metadata
    populateMetadataWithField(metadata, field, writeName);
    // Create lon and lat
    std::vector<atlas::PointLonLat> atlasLonLat = utilsatlas::getAtlasCoords(field);
    std::vector<std::shared_ptr<DataContainerBase>> coordContainers =
              utilsatlas::convertLatLonToContainers(atlasLonLat, consts::kCoordVarNames);
    for (const auto& coordContainer : coordContainers) {
      data.addContainer(coordContainer);
    }
    std::shared_ptr<monio::Variable> lonVar = std::make_shared<Variable>(
                  consts::kCoordVarNames[consts::eLongitude], consts::eDouble);
    std::shared_ptr<monio::Variable> latVar = std::make_shared<Variable>(
                  consts::kCoordVarNames[consts::eLatitude], consts::eDouble);
    std::string dimName = metadata.getDimensionName(atlasLonLat.size());
    lonVar->addDimension(dimName, atlasLonLat.size());
    latVar->addDimension(dimName, atlasLonLat.size());
    metadata.addVariable(consts::kCoordVarNames[consts::eLongitude], lonVar);
    metadata.addVariable(consts::kCoordVarNames[consts::eLatitude], latVar);

    populateDataWithField(data, field, fieldShape);
    addGlobalAttributes(metadata, false);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void monio::AtlasWriter::populateMetadataWithField(Metadata& metadata,
                                             const atlas::Field& field,
                                             const consts::FieldMetadata& fieldMetadata,
                                             const std::string& varName,
                                             const std::string& vertConfigName) {
  oops::Log::trace() << "AtlasWriter::populateMetadataWithField()" << std::endl;
  int type = utilsatlas::atlasTypeToMonioEnum(field.datatype());
  std::shared_ptr<monio::Variable> var = std::make_shared<Variable>(varName, type);
  // Variable dimensions
  addVariableDimensions(field, metadata, var, vertConfigName);
  // Variable attributes
  for (int i = 0; i < consts::eNumberOfAttributeNames; ++i) {
    std::string attributeName = std::string(consts::kIncrementAttributeNames[i]);
    std::string attributeValue;
    switch (i) {
      case consts::eStandardName:
        attributeValue = fieldMetadata.jediName;
        break;
      case consts::eLongName:
        attributeValue = fieldMetadata.jediName + "_inc";
        break;
      case consts::eUnitsName:
        attributeValue = fieldMetadata.units;
        break;
      default:
        attributeValue = consts::kIncrementVariableValues[i];
    }
    std::shared_ptr<AttributeBase> incAttr = std::make_shared<AttributeString>(attributeName,
                                                                               attributeValue);
    var->addAttribute(incAttr);
  }
  metadata.addVariable(varName, var);
}

void monio::AtlasWriter::populateMetadataWithField(Metadata& metadata,
                                             const atlas::Field& field,
                                             const std::string& varName) {
  oops::Log::trace() << "AtlasWriter::populateMetadataWithField()" << std::endl;
  int type = utilsatlas::atlasTypeToMonioEnum(field.datatype());
  std::shared_ptr<monio::Variable> var = std::make_shared<Variable>(varName, type);
  // Variable dimensions
  addVariableDimensions(field, metadata, var);
  metadata.addVariable(varName, var);
}

void monio::AtlasWriter::populateDataWithField(Data& data,
                                         const atlas::Field& field,
                                         const std::vector<size_t>& lfricToAtlasMap,
                                         const std::string& fieldName) {
  oops::Log::trace() << "AtlasWriter::populateDataWithField()" << std::endl;
  std::shared_ptr<DataContainerBase> dataContainer = nullptr;
  populateDataContainerWithField(dataContainer, field, lfricToAtlasMap, fieldName);
  data.addContainer(dataContainer);
}

void monio::AtlasWriter::populateDataWithField(Data& data,
                                         const atlas::Field& field,
                                         const std::vector<atlas::idx_t>& dimensions) {
  oops::Log::trace() << "AtlasWriter::populateDataWithField()" << std::endl;
  std::shared_ptr<DataContainerBase> dataContainer = nullptr;
  populateDataContainerWithField(dataContainer, field, dimensions);
  data.addContainer(dataContainer);
}

void monio::AtlasWriter::populateDataContainerWithField(
                                     std::shared_ptr<monio::DataContainerBase>& dataContainer,
                               const atlas::Field& field,
                               const std::vector<size_t>& lfricToAtlasMap,
                               const std::string& fieldName) {
  oops::Log::trace() << "AtlasWriter::populateDataContainerWithField()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    atlas::array::DataType atlasType = field.datatype();
    atlas::idx_t fieldSize = utilsatlas::getGlobalDataSize(field);
    switch (atlasType.kind()) {
      case atlasType.KIND_INT32: {
        if (dataContainer == nullptr) {
          dataContainer = std::make_shared<DataContainerInt>(fieldName);
        }
        std::shared_ptr<DataContainerInt> dataContainerInt =
                          std::static_pointer_cast<DataContainerInt>(dataContainer);
        dataContainerInt->clear();
        dataContainerInt->setSize(fieldSize);
        populateDataVec(dataContainerInt->getData(), field, lfricToAtlasMap);
        break;
      }
      case atlasType.KIND_REAL32: {
        if (dataContainer == nullptr) {
          dataContainer = std::make_shared<DataContainerFloat>(fieldName);
        }
        std::shared_ptr<DataContainerFloat> dataContainerFloat =
                          std::static_pointer_cast<DataContainerFloat>(dataContainer);
        dataContainerFloat->clear();
        dataContainerFloat->setSize(fieldSize);
        populateDataVec(dataContainerFloat->getData(), field, lfricToAtlasMap);
        break;
      }
      case atlasType.KIND_REAL64: {
        if (dataContainer == nullptr) {
          dataContainer = std::make_shared<DataContainerDouble>(fieldName);
        }
        std::shared_ptr<DataContainerDouble> dataContainerDouble =
                          std::static_pointer_cast<DataContainerDouble>(dataContainer);
        dataContainerDouble->clear();
        dataContainerDouble->setSize(fieldSize);
        populateDataVec(dataContainerDouble->getData(), field, lfricToAtlasMap);
        break;
      }
      default: {
        Monio::get().closeFiles();
        utils::throwException("AtlasWriter::populateDataContainerWithField()> "
                                 "Data type not coded for...");
      }
    }
  }
}

void monio::AtlasWriter::populateDataContainerWithField(
                                     std::shared_ptr<monio::DataContainerBase>& dataContainer,
                               const atlas::Field& field,
                               const std::vector<atlas::idx_t>& dimensions) {
  oops::Log::trace() << "AtlasWriter::populateDataContainerWithField()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    std::string fieldName = field.name();
    atlas::array::DataType atlasType = field.datatype();
    atlas::idx_t fieldSize = utilsatlas::getGlobalDataSize(field);
    switch (atlasType.kind()) {
      case atlasType.KIND_INT32: {
        if (dataContainer == nullptr) {
          dataContainer = std::make_shared<DataContainerInt>(fieldName);
        }
        std::shared_ptr<DataContainerInt> dataContainerInt =
                          std::static_pointer_cast<DataContainerInt>(dataContainer);
        dataContainerInt->clear();
        dataContainerInt->setSize(fieldSize);
        populateDataVec(dataContainerInt->getData(), field, dimensions);
        break;
      }
      case atlasType.KIND_REAL32: {
        if (dataContainer == nullptr) {
          dataContainer = std::make_shared<DataContainerFloat>(fieldName);
        }
        std::shared_ptr<DataContainerFloat> dataContainerFloat =
                          std::static_pointer_cast<DataContainerFloat>(dataContainer);
        dataContainerFloat->clear();
        dataContainerFloat->setSize(fieldSize);
        populateDataVec(dataContainerFloat->getData(), field, dimensions);
        break;
      }
      case atlasType.KIND_REAL64: {
        if (dataContainer == nullptr) {
          dataContainer = std::make_shared<DataContainerDouble>(fieldName);
        }
        std::shared_ptr<DataContainerDouble> dataContainerDouble =
                          std::static_pointer_cast<DataContainerDouble>(dataContainer);
        dataContainerDouble->clear();
        dataContainerDouble->setSize(fieldSize);
        populateDataVec(dataContainerDouble->getData(), field, dimensions);
        break;
      }
      default: {
        Monio::get().closeFiles();
        utils::throwException("AtlasWriter::populateDataContainerWithField()> "
                              "Data type not coded for...");
      }
    }
  }
}

template<typename T>
void monio::AtlasWriter::populateDataVec(std::vector<T>& dataVec,
                                   const atlas::Field& field,
                                   const std::vector<size_t>& lfricToAtlasMap) {
  oops::Log::trace() << "AtlasWriter::populateDataVec() " << field.name() << std::endl;
  atlas::idx_t numLevels = field.shape(consts::eVertical);
  if ((lfricToAtlasMap.size() * numLevels) != dataVec.size()) {
    Monio::get().closeFiles();
    utils::throwException("AtlasWriter::populateDataVec()> "
                          "Data container is not configured for the expected data...");
  }
  auto fieldView = atlas::array::make_view<T, 2>(field);
  for (std::size_t i = 0; i < lfricToAtlasMap.size(); ++i) {
    for (atlas::idx_t j = 0; j < numLevels; ++j) {
      atlas::idx_t index = lfricToAtlasMap[i] + (j * lfricToAtlasMap.size());
      dataVec[index] = fieldView(i, j);
    }
  }
}

template void monio::AtlasWriter::populateDataVec<double>(std::vector<double>& dataVec,
                                                    const atlas::Field& field,
                                                    const std::vector<size_t>& lfricToAtlasMap);
template void monio::AtlasWriter::populateDataVec<float>(std::vector<float>& dataVec,
                                                   const atlas::Field& field,
                                                   const std::vector<size_t>& lfricToAtlasMap);
template void monio::AtlasWriter::populateDataVec<int>(std::vector<int>& dataVec,
                                                 const atlas::Field& field,
                                                 const std::vector<size_t>& lfricToAtlasMap);

template<typename T>
void monio::AtlasWriter::populateDataVec(std::vector<T>& dataVec,
                                   const atlas::Field& field,
                                   const std::vector<atlas::idx_t>& dimensions) {
  oops::Log::trace() << "AtlasWriter::populateDataVec()" << std::endl;
  auto fieldView = atlas::array::make_view<T, 2>(field);
  for (atlas::idx_t i = 0; i < dimensions[consts::eHorizontal]; ++i) {
    for (atlas::idx_t j = 0; j < dimensions[consts::eVertical]; ++j) {
      atlas::idx_t index = j + (i * dimensions[consts::eVertical]);
      dataVec[index] = fieldView(i, j);
    }
  }
}

template void monio::AtlasWriter::populateDataVec<double>(std::vector<double>& dataVec,
                                                    const atlas::Field& field,
                                                    const std::vector<atlas::idx_t>& dimensions);
template void monio::AtlasWriter::populateDataVec<float>(std::vector<float>& dataVec,
                                                   const atlas::Field& field,
                                                   const std::vector<atlas::idx_t>& dimensions);
template void monio::AtlasWriter::populateDataVec<int>(std::vector<int>& dataVec,
                                                 const atlas::Field& field,
                                                 const std::vector<atlas::idx_t>& dimensions);

atlas::Field monio::AtlasWriter::getWriteField(atlas::Field& field,
                                         const std::string& writeName,
                                         const bool noFirstLevel) {
  oops::Log::trace() << "AtlasWriter::getWriteField()" << std::endl;
  atlas::FunctionSpace functionSpace = field.functionspace();
  atlas::array::DataType atlasType = field.datatype();
  if (atlasType != atlasType.KIND_REAL64 &&
      atlasType != atlasType.KIND_REAL32 &&
      atlasType != atlasType.KIND_INT32) {
      Monio::get().closeFiles();
      utils::throwException("AtlasWriter::getWriteField())> Data type not coded for...");
  }
  atlas::idx_t numLevels = field.shape(consts::eVertical);
  // Erroneous case. For noFirstLevel == true field should have 70 levels
  if (noFirstLevel == true && numLevels == consts::kVerticalFullSize) {
    Monio::get().closeFiles();
    utils::throwException("AtlasWriter::getWriteField()> Field levels misconfiguration...");
  }
  // WARNING - This name-check is an LFRic-Lite specific convention...
  if (utils::findInVector(consts::kMissingVariableNames, writeName) == false) {
    if (noFirstLevel == true && numLevels == consts::kVerticalHalfSize) {
      atlas::util::Config atlasOptions = atlas::option::name(writeName) |
                                         atlas::option::global(0) |
                                         atlas::option::levels(consts::kVerticalFullSize);
      switch (atlasType.kind()) {
        case atlasType.KIND_REAL64: {
          return copySurfaceLevel<double>(field, functionSpace, atlasOptions);
        }
        case atlasType.KIND_REAL32: {
          return copySurfaceLevel<float>(field, functionSpace, atlasOptions);
        }
        case atlasType.KIND_INT32: {
          return copySurfaceLevel<int>(field, functionSpace, atlasOptions);
        }
      }
    } else {
      field.metadata().set("name", writeName);
    }
  } else {
    Monio::get().closeFiles();
    utils::throwException("AtlasWriter::getWriteField()> Field write name misconfiguration...");
  }
  return field;
}

template<typename T>
atlas::Field monio::AtlasWriter::copySurfaceLevel(const atlas::Field& inputField,
                              const atlas::FunctionSpace& functionSpace,
                              const atlas::util::Config& atlasOptions) {
  oops::Log::trace() << "AtlasWriter::copySurfaceLevel()" << std::endl;
  atlas::Field copiedField = functionSpace.createField<T>(atlasOptions);
  auto copiedFieldView = atlas::array::make_view<T, 2>(copiedField);
  auto inputFieldView = atlas::array::make_view<T, 2>(inputField);
  std::vector<atlas::idx_t> fieldShape = inputField.shape();
  for (atlas::idx_t j = 0; j < fieldShape[consts::eVertical]; ++j) {
    for (atlas::idx_t i = 0; i < fieldShape[consts::eHorizontal]; ++i) {
      copiedFieldView(i, j + 1) = inputFieldView(i, j);
    }
  }
  // Copy surface level of input field
  for (atlas::idx_t i = 0; i < fieldShape[consts::eHorizontal]; ++i) {
    copiedFieldView(i, 0) = inputFieldView(i, 0);
  }
  return copiedField;
}

template atlas::Field monio::AtlasWriter::copySurfaceLevel<double>(const atlas::Field& inputField,
                                                          const atlas::FunctionSpace& functionSpace,
                                                          const atlas::util::Config& atlasOptions);
template atlas::Field monio::AtlasWriter::copySurfaceLevel<float>(const atlas::Field& inputField,
                                                          const atlas::FunctionSpace& functionSpace,
                                                          const atlas::util::Config& atlasOptions);
template atlas::Field monio::AtlasWriter::copySurfaceLevel<int>(const atlas::Field& inputField,
                                                          const atlas::FunctionSpace& functionSpace,
                                                          const atlas::util::Config& atlasOptions);

void monio::AtlasWriter::addVariableDimensions(const atlas::Field& field,
                                               const Metadata& metadata,
                                                     std::shared_ptr<monio::Variable> var,
                                               const std::string& vertConfigName) {
  std::vector<atlas::idx_t> fieldShape = field.shape();
  if (field.metadata().get<bool>("global") == false) {  // If so, get the 2D size of the Field
    fieldShape[consts::eHorizontal] = utilsatlas::getHorizontalSize(field);
  }
  // Reversal of dims required for LFRic files. Currently applied to all output files.
  std::reverse(fieldShape.begin(), fieldShape.end());
  for (auto& dimSize : fieldShape) {
    std::string dimName;
    if (vertConfigName != "" && metadata.isDimDefined(vertConfigName) &&
          dimSize == metadata.getDimension(vertConfigName)) {
      dimName = vertConfigName;
    } else {
      dimName = metadata.getDimensionName(dimSize);
    }
    if (dimName != consts::kNotFoundError) {  // Not used for 1-D fields.
      var->addDimension(dimName, dimSize);
    }
  }
}

void monio::AtlasWriter::addGlobalAttributes(Metadata& metadata, const bool isLfricConvention) {
  // Initialise variables
  std::string variableConvention =
      isLfricConvention == true ? consts::kNamingConventions[consts::eLfricConvention] :
                                  consts::kNamingConventions[consts::eJediConvention];
  // Create attribute objects
  std::shared_ptr<monio::AttributeString> namingAttr =
      std::make_shared<AttributeString>(std::string(consts::kVariableConventionName),
                                        variableConvention);
  std::shared_ptr<monio::AttributeString> producedByAttr =
      std::make_shared<AttributeString>(std::string(consts::kProducedByName),
                                        std::string(consts::kProducedByString));
  // Add to metadata
  metadata.addGlobalAttr(namingAttr->getName(), namingAttr);
  metadata.addGlobalAttr(producedByAttr->getName(), producedByAttr);
}
