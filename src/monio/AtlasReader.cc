/******************************************************************************
* MONIO - Met Office NetCDF Input Output                                      *
*                                                                             *
* (C) Crown Copyright 2023, Met Office. All rights reserved.                  *
*                                                                             *
* This software is licensed under the terms of the 3-Clause BSD License       *
* which can be obtained from https://opensource.org/license/bsd-3-clause/.    *
******************************************************************************/
#include "AtlasReader.h"

#include "oops/util/Logger.h"

#include "Utils.h"
#include "UtilsAtlas.h"
#include "Monio.h"

monio::AtlasReader::AtlasReader(const eckit::mpi::Comm& mpiCommunicator, const int mpiRankOwner):
    mpiCommunicator_(mpiCommunicator),
    mpiRankOwner_(mpiRankOwner) {
  oops::Log::trace() << "AtlasReader::AtlasReader()" << std::endl;
}

void monio::AtlasReader::populateFieldWithFileData(atlas::Field& field,
                                             const FileData& fileData,
                                             const consts::FieldMetadata& fieldMetadata,
                                             const std::string& readName,
                                             const bool isLfricConvention) {
  oops::Log::trace() << "AtlasReader::populateFieldWithFileData()" << std::endl;
  atlas::Field readField = getReadField(field, fieldMetadata.noFirstLevel);
  populateFieldWithDataContainer(readField,
                                 fileData.getData().getContainer(readName),
                                 fileData.getLfricAtlasMap(),
                                 fieldMetadata.noFirstLevel,
                                 isLfricConvention);
}

void monio::AtlasReader::populateFieldWithDataContainer(atlas::Field& field,
                                      const std::shared_ptr<DataContainerBase>& dataContainer,
                                      const std::vector<size_t>& lfricToAtlasMap,
                                      const bool noFirstLevel,
                                      const bool isLfricConvention) {
  oops::Log::trace() << "AtlasReader::populateFieldWithDataContainer()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    int dataType = dataContainer.get()->getType();
    switch (dataType) {
      case consts::eDataTypes::eDouble: {
        const std::shared_ptr<DataContainerDouble> dataContainerDouble =
            std::static_pointer_cast<DataContainerDouble>(dataContainer);
            populateField(field, dataContainerDouble->getData(), lfricToAtlasMap,
                          noFirstLevel, isLfricConvention);
        break;
      }
      case consts::eDataTypes::eFloat: {
        const std::shared_ptr<DataContainerFloat> dataContainerFloat =
            std::static_pointer_cast<DataContainerFloat>(dataContainer);
            populateField(field, dataContainerFloat->getData(), lfricToAtlasMap,
                          noFirstLevel, isLfricConvention);
        break;
      }
      case consts::eDataTypes::eInt: {
        const std::shared_ptr<DataContainerInt> dataContainerInt =
            std::static_pointer_cast<DataContainerInt>(dataContainer);
            populateField(field, dataContainerInt->getData(), lfricToAtlasMap,
                          noFirstLevel, isLfricConvention);
        break;
      }
      default: {
        Monio::get().closeFiles();
        utils::throwException("AtlasReader::populateFieldWithDataContainer()> "
                                 "Data type not coded for...");
      }
    }
  }
}

void monio::AtlasReader::populateFieldWithDataContainer(atlas::Field& field,
                                      const std::shared_ptr<DataContainerBase>& dataContainer) {
  oops::Log::trace() << "AtlasReader::populateFieldWithDataContainer()" << std::endl;
  if (mpiCommunicator_.rank() == mpiRankOwner_) {
    int dataType = dataContainer.get()->getType();
    switch (dataType) {
      case consts::eDataTypes::eDouble: {
        const std::shared_ptr<DataContainerDouble> dataContainerDouble =
            std::static_pointer_cast<DataContainerDouble>(dataContainer);
            populateField(field, dataContainerDouble->getData());
        break;
      }
      case consts::eDataTypes::eFloat: {
        const std::shared_ptr<DataContainerFloat> dataContainerFloat =
            std::static_pointer_cast<DataContainerFloat>(dataContainer);
            populateField(field, dataContainerFloat->getData());
        break;
      }
      case consts::eDataTypes::eInt: {
        const std::shared_ptr<DataContainerInt> dataContainerInt =
            std::static_pointer_cast<DataContainerInt>(dataContainer);
            populateField(field, dataContainerInt->getData());
        break;
      }
      default: {
        Monio::get().closeFiles();
        utils::throwException("AtlasReader::populateFieldWithDataContainer()> "
                                 "Data type not coded for...");
      }
    }
  }
}

template<typename T>
void monio::AtlasReader::populateField(atlas::Field& field,
                                 const std::vector<T>& dataVec,
                                 const std::vector<size_t>& lfricToAtlasMap,
                                 const bool noFirstLevel,
                                 const bool isLfricConvention) {
  oops::Log::trace() << "AtlasReader::populateField()" << std::endl;
  auto fieldView = atlas::array::make_view<T, 2>(field);
  // Field with noFirstLevel == true should have been adjusted to have 70 levels.
  atlas::idx_t numLevels = field.shape(consts::eVertical);
  if (noFirstLevel == true && numLevels == consts::kVerticalFullSize) {
    Monio::get().closeFiles();
    utils::throwException("AtlasReader::populateField()> Field levels misconfiguration...");
  // Only valid case for field with noFirstLevel == true. Field is adjusted to have 70 levels but
  // read data still has enough to fill 71.
  } else if (isLfricConvention == true &&
             noFirstLevel == true &&
             numLevels == consts::kVerticalHalfSize) {
    for (int j = 1; j < consts::kVerticalFullSize; ++j) {
      for (std::size_t i = 0; i < lfricToAtlasMap.size(); ++i) {
        int index = lfricToAtlasMap[i] + (j * lfricToAtlasMap.size());
        // Bounds checking
        if (std::size_t(index) <= dataVec.size()) {
          fieldView(i, j - 1) = dataVec[index];
        } else {
          Monio::get().closeFiles();
          utils::throwException("AtlasReader::populateField()> Calculated index exceeds size of "
                                "data for field \"" + field.name() + "\".");
        }
      }
    }
  // Valid case for isLfricConvention == false and fields noFirstLevel == false. Field is filled
  // with all available data.
  } else {
    for (atlas::idx_t j = 0; j < numLevels; ++j) {
      for (std::size_t i = 0; i < lfricToAtlasMap.size(); ++i) {
        int index = lfricToAtlasMap[i] + (j * lfricToAtlasMap.size());
        // Bounds checking
        if (std::size_t(index) <= dataVec.size()) {
          fieldView(i, j) = dataVec[index];
        } else {
          Monio::get().closeFiles();
          utils::throwException("AtlasReader::populateField()> Calculated index exceeds size of "
                                "data for field \"" + field.name() + "\".");
        }
      }
    }
  }
}

template void monio::AtlasReader::populateField<double>(atlas::Field& field,
                                                        const std::vector<double>& dataVec,
                                                        const std::vector<size_t>& lfricToAtlasMap,
                                                        const bool copyFirstLevel,
                                                        const bool isLfricConvention);
template void monio::AtlasReader::populateField<float>(atlas::Field& field,
                                                       const std::vector<float>& dataVec,
                                                       const std::vector<size_t>& lfricToAtlasMap,
                                                       const bool copyFirstLevel,
                                                       const bool isLfricConvention);
template void monio::AtlasReader::populateField<int>(atlas::Field& field,
                                                     const std::vector<int>& dataVec,
                                                     const std::vector<size_t>& lfricToAtlasMap,
                                                     const bool copyFirstLevel,
                                                     const bool isLfricConvention);

template<typename T>
void monio::AtlasReader::populateField(atlas::Field& field,
                                       const std::vector<T>& dataVec) {
  oops::Log::trace() << "AtlasReader::populateField()" << std::endl;

  std::vector<atlas::idx_t> fieldShape = field.shape();
  if (field.metadata().get<bool>("global") == false) {
    fieldShape[consts::eHorizontal] = utilsatlas::getHorizontalSize(field);
  }
  auto fieldView = atlas::array::make_view<T, 2>(field);
  for (atlas::idx_t i = 0; i < fieldShape[consts::eHorizontal]; ++i) {
    for (atlas::idx_t j = 0; j < fieldShape[consts::eVertical]; ++j) {
      atlas::idx_t index = i + (j * fieldShape[consts::eHorizontal]);
      if (std::size_t(index) <= dataVec.size()) {
        fieldView(i, j) = dataVec[index];
      } else {
        Monio::get().closeFiles();
        utils::throwException("AtlasReader::populateField()> "
                              "Calculated index exceeds size of data.");
      }
    }
  }
}

template void monio::AtlasReader::populateField<double>(atlas::Field& field,
                                                        const std::vector<double>& dataVec);
template void monio::AtlasReader::populateField<float>(atlas::Field& field,
                                                       const std::vector<float>& dataVec);
template void monio::AtlasReader::populateField<int>(atlas::Field& field,
                                                     const std::vector<int>& dataVec);

atlas::Field monio::AtlasReader::getReadField(atlas::Field& field,
                                              const bool noFirstLevel) {
  // Check to ensure field has not been initialised with 71 levels
  if (noFirstLevel == true && field.shape(consts::eVertical) == consts::kVerticalFullSize) {
    atlas::array::DataType atlasType = field.datatype();
    if (atlasType != atlasType.KIND_REAL64 &&
        atlasType != atlasType.KIND_REAL32 &&
        atlasType != atlasType.KIND_INT32) {
        Monio::get().closeFiles();
        utils::throwException("AtlasReader::getReadField())> Data type not coded for...");
    }
    atlas::util::Config atlasOptions = atlas::option::name(field.name()) |
                                       atlas::option::levels(consts::kVerticalHalfSize) |
                                       atlas::option::datatype(atlasType) |
                                       atlas::option::global(0);
    const auto& functionSpace = field.functionspace();
    return functionSpace.createField(atlasOptions);
  } else {
    return field;  // Case when field is initialised correctly.
  }
}
