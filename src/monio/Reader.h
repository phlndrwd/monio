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
#include <tuple>
#include <vector>

#include "eckit/mpi/Comm.h"
#include "oops/util/DateTime.h"

#include "DataContainerBase.h"
#include "File.h"
#include "FileData.h"

namespace monio {
/// \brief Top-level class uses File, Metadata,
/// and Data to read from a NetCDF file
class Reader {
 public:
  Reader(const eckit::mpi::Comm& mpiCommunicator,
         const atlas::idx_t mpiRankOwner,
         const FileData& fileData);

  Reader(const eckit::mpi::Comm& mpiCommunicator,
         const atlas::idx_t mpiRankOwner);

  Reader()                         = delete;  //!< Deleted default constructor
  Reader(Reader&&)                 = delete;  //!< Deleted move constructor
  Reader(const Reader&)            = delete;  //!< Deleted copy constructor
  Reader& operator=(Reader&&)      = delete;  //!< Deleted move assignment
  Reader& operator=(const Reader&) = delete;  //!< Deleted copy assignment

  void openFile(const FileData& fileData);

  void readMetadata(FileData& fileData);
  void readAllData(FileData& fileData);

  void readSingleData(FileData& fileData, const std::vector<std::string>& varNames);
  void readSingleDatum(FileData& fileData, const std::string& varName);

  void readFieldData(FileData& fileData,
                     const std::vector<std::string>& variableNames,
                     const std::string& dateString,
                     const std::string& timeDimName);

  void readFieldData(FileData& fileData,
                     const std::vector<std::string>& variableNames,
                     const util::DateTime& dateToRead,
                     const std::string& timeDimName);

  void readFieldDatum(FileData& fileData,
                      const std::string& variableName,
                      const util::DateTime& dateToRead,
                      const std::string& timeDimName);

  void readFieldDatum(FileData& fileData,
                      const std::string& variableName,
                      const size_t timeStep,
                      const std::string& timeDimName);

  std::vector<std::string> getVarStrAttrs(const FileData& fileData,
                                          const std::vector<std::string>& varNames,
                                          const std::string& attrName);

  std::vector<std::shared_ptr<DataContainerBase>> getCoordData(FileData& fileData,
                                                  const std::vector<std::string>& coordNames);
  // The following function takes a levels 'search term' as some variables use full- or half-levels
  // This approach allows the correct number of levels for the variable to be determined
  std::vector<monio::constants::FieldMetadata> getFieldMetadata(const FileData& fileData,
                                           const std::vector<std::string>& lfricFieldNames,
                                           const std::vector<std::string>& atlasFieldNames,
                                           const std::string& levelsSearchTerm);

 private:
  size_t getSizeOwned(const FileData& fileData, const std::string& varName);
  size_t getVarNumLevels(const FileData& fileData,
                         const std::string& varName,
                         const std::string& levelsSearchTerm);
  int getVarDataType(const FileData& fileData, const std::string& varName);
  size_t findTimeStep(const FileData& fileData, const util::DateTime& dateTime);

  File& getFile();

  const eckit::mpi::Comm& mpiCommunicator_;
  const std::size_t mpiRankOwner_;

  std::unique_ptr<File> file_;
};
}  // namespace monio
