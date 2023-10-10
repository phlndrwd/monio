/******************************************************************************
* MONIO - Met Office NetCDF Input Output                                      *
*                                                                             *
* (C) Crown Copyright 2023 Met Office                                         *
*                                                                             *
* This software is licensed under the terms of the Apache Licence Version 2.0 *
* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.        *
******************************************************************************/
#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Constants.h"
#include "FileData.h"

#include "atlas/array/DataType.h"
#include "atlas/field.h"
#include "atlas/functionspace/CubedSphereColumns.h"
#include "atlas/grid/CubedSphereGrid.h"
#include "atlas/util/Point.h"
#include "eckit/mpi/Comm.h"

namespace monio {
  /// \brief Used during file writing. Encapsulates the dependency upon Atlas. Includes functions to
  ///        populate data containers with data in Atlas fields.
class AtlasWriter {
 public:
  AtlasWriter(const eckit::mpi::Comm& mpiCommunicator,
              const int mpiRankOwner);

  AtlasWriter()                               = delete;  //!< Deleted default constructor
  AtlasWriter(AtlasWriter&&)                  = delete;  //!< Deleted move constructor
  AtlasWriter(const AtlasWriter&)             = delete;  //!< Deleted copy constructor
  AtlasWriter& operator=( AtlasWriter&&)      = delete;  //!< Deleted move assignment
  AtlasWriter& operator=(const AtlasWriter&)  = delete;  //!< Deleted copy assignment

  /// \brief Creates required metadata and data from at Atlas field. For writing LFRic data with
  ///        some existing metadata.
  void populateFileDataWithField(FileData& fileData,
                                 atlas::Field& field,
                           const consts::FieldMetadata& fieldMetadata,
                           const std::string& writeName,
                           const bool isLfricNaming);

  /// \brief Creates all metadata and data from at Atlas field. For writing of field sets with no
  ///        metadata.
  void populateFileDataWithField(FileData& fileData,
                           const atlas::Field& field,
                           const std::string& writeName);

 private:
  /// \brief Called from the entry point with LFRic metadata.
  void populateMetadataWithField(Metadata& metadata,
                           const atlas::Field& field,
                           const consts::FieldMetadata& fieldMetadata,
                           const std::string& varName);

  /// \brief Called from the entry point with no existing metadata.
  void populateMetadataWithField(Metadata& metadata,
                           const atlas::Field& field,
                           const std::string& varName);

  /// \brief Called from the entry point with LFRic metadata.
  void populateDataWithField(Data& data,
                       const atlas::Field& field,
                       const std::vector<size_t>& lfricToAtlasMap,
                       const std::string& fieldName);

  /// \brief Called from the entry point with no existing metadata.
  void populateDataWithField(Data& data,
                       const atlas::Field& field,
                       const std::vector<int> dimensions);

  /// \brief With LFRic metadata, derives the container type and makes the call to populate it.
  void populateDataContainerWithField(std::shared_ptr<monio::DataContainerBase>& dataContainer,
                                const atlas::Field& field,
                                const std::vector<size_t>& lfricToAtlasMap,
                                const std::string& fieldName);

  /// \brief Without metadata, derives the container type and makes the call to populate it.
  void populateDataContainerWithField(std::shared_ptr<monio::DataContainerBase>& dataContainer,
                                const atlas::Field& field,
                                const std::vector<int>& dimensions);

  /// \brief Iterates through field and populates vector with data from field in LFRic order.
  template<typename T> void populateDataVec(std::vector<T>& dataVec,
                                      const atlas::Field& field,
                                      const std::vector<size_t>& lfricToAtlasMap);

  /// \brief Iterates through field and populates vector with data from field in Atlas order.
  template<typename T> void populateDataVec(std::vector<T>& dataVec,
                                      const atlas::Field& field,
                                      const std::vector<int>& dimensions);

  /// \brief Returns a field formatted for writing to file.
  atlas::Field getWriteField(atlas::Field& inputField,
                       const std::string& writeName,
                       const bool noFirstLevel);

  /// \brief Returns a field with a copy of the zeroth level of input field.
  template<typename T>
  atlas::Field copySurfaceLevel(const atlas::Field& inputField,
                                const atlas::FunctionSpace& functionSpace,
                                const atlas::util::Config& atlasOptions);

  /// \brief Associates a given variable with its applicable dimensions in the metadata.
  void addVariableDimensions(const atlas::Field& field,
                             const Metadata& metadata,
                                   std::shared_ptr<monio::Variable> var);

  void addGlobalAttributes(Metadata& metadata, const bool isLfricNaming = true);

  const eckit::mpi::Comm& mpiCommunicator_;
  const std::size_t mpiRankOwner_;

  /// \brief Used for automatic creation of dimension names for fields with no existing metadata.
  int dimCount_ = 0;
};
}  // namespace monio
