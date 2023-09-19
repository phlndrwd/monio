/*#############################################################################
# MONIO - Met Office NetCDF Input Output                                      #
#                                                                             #
# (C) Crown Copyright 2023 Met Office                                         #
#                                                                             #
# This software is licensed under the terms of the Apache Licence Version 2.0 #
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.        #
#############################################################################*/
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
class AtlasWriter {
 public:
  AtlasWriter(const eckit::mpi::Comm& mpiCommunicator,
              const int mpiRankOwner);

  AtlasWriter()                               = delete;  //!< Deleted default constructor
  AtlasWriter(AtlasWriter&&)                  = delete;  //!< Deleted move constructor
  AtlasWriter(const AtlasWriter&)             = delete;  //!< Deleted copy constructor
  AtlasWriter& operator=( AtlasWriter&&)      = delete;  //!< Deleted move assignment
  AtlasWriter& operator=(const AtlasWriter&)  = delete;  //!< Deleted copy assignment

  // For writing of FieldSets with no metadata
  void populateFileDataWithField(FileData& fileData,
                           const atlas::Field& field,
                           const std::string& writeName);

  // For writing LFRic data with some existing metadata
  void populateFileDataWithField(FileData& fileData,
                                 atlas::Field& field,
                           const consts::FieldMetadata& fieldMetadata,
                           const std::string& writeName,
                           const bool isLfricFormat);

 private:
  void populateDataContainerWithField(std::shared_ptr<monio::DataContainerBase>& dataContainer,
                                const atlas::Field& field,
                                const std::vector<size_t>& lfricToAtlasMap,
                                const std::string& fieldName);

  void populateDataContainerWithField(std::shared_ptr<monio::DataContainerBase>& dataContainer,
                                const atlas::Field& field,
                                const std::vector<int>& dimensions);


  void populateMetadataWithField(Metadata& metadata,
                           const atlas::Field& field,
                           const consts::FieldMetadata& fieldMetadata,
                           const std::string& varName);

  void populateMetadataWithField(Metadata& metadata,
                           const atlas::Field& field,
                           const std::string& varName);

  void populateDataWithField(Data& data,
                       const atlas::Field& field,
                       const std::vector<size_t>& lfricToAtlasMap,
                       const std::string& fieldName);

  void populateDataWithField(Data& data,
                       const atlas::Field& field,
                       const std::vector<int> dimensions);

  template<typename T> void populateDataVec(std::vector<T>& dataVec,
                                      const atlas::Field& field,
                                      const std::vector<size_t>& lfricToAtlasMap);

  template<typename T> void populateDataVec(std::vector<T>& dataVec,
                                      const atlas::Field& field,
                                      const std::vector<int>& dimensions);

  atlas::Field getWriteField(atlas::Field& inputField,
                       const std::string& writeName,
                       const bool noFirstLevel);

  template<typename T>
  atlas::Field copySurfaceLevel(const atlas::Field& inputField,
                                const atlas::FunctionSpace& functionSpace,
                                const atlas::util::Config& atlasOptions);

  void addVariableDimensions(const atlas::Field& field,
                             const Metadata& metadata,
                                   std::shared_ptr<monio::Variable> var);

  void addGlobalAttributes(Metadata& metadata, const bool isLFRicFormat = true);

  const eckit::mpi::Comm& mpiCommunicator_;
  const std::size_t mpiRankOwner_;

  int dimCount_ = 0;  // Used for automatic creation of dimension names
};
}  // namespace monio
