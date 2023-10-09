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
#include <utility>
#include <vector>

#include "AtlasReader.h"
#include "AtlasWriter.h"
#include "FileData.h"
#include "Reader.h"
#include "Writer.h"

namespace monio {
/// \brief Provides functions for the main use-cases of MONIO in the MO/JEDI context. Including two
///        that are written for use in debugging and testing only. All are available via a global,
///        singleton instance of this class.
class Monio {
 public:
  /// \brief The main singleton getter for Monio.
  static Monio& get();

  ~Monio();

  Monio()                        = delete;  //!< Deleted default constructor
  Monio(Monio&&)                 = delete;  //!< Deleted move constructor
  Monio(const Monio&)            = delete;  //!< Deleted copy constructor
  Monio& operator=(Monio&&)      = delete;  //!< Deleted move assignment
  Monio& operator=(const Monio&) = delete;  //!< Deleted copy assignment

  /// \brief Reads files with a time component, i.e. state files.
  void readState(atlas::FieldSet& localFieldSet,
           const std::vector<consts::FieldMetadata>& fieldMetadataVec,
           const std::string& filePath,
           const util::DateTime& dateTime);

  /// \brief Reads files without a time component, i.e. increment files.
  void readIncrements(atlas::FieldSet& localFieldSet,
                const std::vector<consts::FieldMetadata>& fieldMetadataVec,
                const std::string& filePath);

  /// \brief Writes increment files. No time component but the variables can use JEDI or LFRic write
  ///        names.
  void writeIncrements(const atlas::FieldSet& localFieldSet,
                       const std::vector<consts::FieldMetadata>& fieldMetadataVec,
                       const std::string& filePath,
                       const bool isLfricNaming = true);

  /// \brief Writes increment files. No time component but the variables can use JEDI or LFRic read
  ///        names. Intended debugging and testing only.
  void writeState(const atlas::FieldSet& localFieldSet,
                  const std::vector<consts::FieldMetadata>& fieldMetadataVec,
                  const std::string& filePath,
                  const bool isLfricNaming = true);

  /// \brief Writes an instance of our field sets to file. Intended debugging and testing only.
  void writeFieldSet(const atlas::FieldSet& localFieldSet,
                     const std::string& filePath);

  /// \brief Called when handling exceptions elsewhere in MONIO to free disk resources more quickly.
  void closeFiles();

  /// \brief A call to open and initialise a state file for reading. This function is public whilst
  ///        it's called from LFRic-Lite.
  int initialiseFile(const atlas::Grid& grid,
                     const std::string& filePath,
                     const util::DateTime& dateTime);

 private:
  /// \brief Private class constructor to prevent instantiation outside of the singleton.
  Monio(const eckit::mpi::Comm& mpiCommunicator,
        const int mpiRankOwner);

  /// \brief Creates and returns an instance of FileData from a state file for a given grid
  ///        resolution.
  FileData& createFileData(const std::string& gridName,
                           const std::string& filePath,
                           const util::DateTime& dateTime);

  /// \brief Creates and returns an instance of FileData from an increment file for a given grid
  ///        resolution.
  FileData& createFileData(const std::string& gridName,
                           const std::string& filePath);

  /// \brief A call to open and initialise an increment file for reading.
  int initialiseFile(const atlas::Grid& grid,
                     const std::string& filePath);  // Public, whilst called from LFRic-Lite

  /// \brief Creates and stores a map between Atlas and LFRic horizontal ordering.
  void createLfricAtlasMap(FileData& fileData, const atlas::CubedSphereGrid& grid);

  /// \brief Creates and stores date-times from a state file.
  void createDateTimes(FileData& fileData,
                       const std::string& timeVarName,
                       const std::string& timeOriginName);

  /// \brief Returns a copy of the data read and produced during file initialisation.
  FileData getFileData(const std::string& gridName);

  /// \brief Removes unnecessary meta/data from data read from file during initialisation.
  void cleanFileData(FileData& fileData);

  /// \brief Necessary use of a standard pointer to a single instance of this class (as part of the
  ///        singleton pattern). Previous use of a smart pointer appeared to cause errors in HDF5
  ///        upon destruction.
  static Monio* this_;

  /// \brief A reference to the MPI communicator passed in at construction.
  const eckit::mpi::Comm& mpiCommunicator_;
  /// \brief A constant to define the single PE rank used to handle the bulk of MONIO I/O.
  const std::size_t mpiRankOwner_;

  /// \brief A member instance of Reader.
  Reader reader_;
  /// \brief A member instance of Writer.
  Writer writer_;

  /// \brief A member instance of AtlasReader.
  AtlasReader atlasReader_;
  /// \brief A member instance of AtlasWriter.
  AtlasWriter atlasWriter_;

  /// \brief Store of read file meta/data used for writing. Keyed by grid name for storage of data
  ///        at different resolutions.
  std::map<std::string, monio::FileData> filesData_;
};
}  // namespace monio
