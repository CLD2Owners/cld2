// Copyright 2014 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <assert.h>
#include <stdio.h>
#include <fstream>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include "cld2_dynamic_data.h"
#include "cld2_dynamic_data_extractor.h"
#include "cld2_dynamic_data_loader.h"
#include "integral_types.h"
#include "cld2tablesummary.h"
#include "utf8statetable.h"
#include "scoreonescriptspan.h"

// We need these in order to set up a real data object to pass around.                              
namespace CLD2 {
  extern const UTF8PropObj cld_generated_CjkUni_obj;
  extern const CLD2TableSummary kCjkCompat_obj;
  extern const CLD2TableSummary kCjkDeltaBi_obj;
  extern const CLD2TableSummary kDistinctBiTable_obj;
  extern const CLD2TableSummary kQuad_obj;
  extern const CLD2TableSummary kQuad_obj2;
  extern const CLD2TableSummary kDeltaOcta_obj;
  extern const CLD2TableSummary kDistinctOcta_obj;
  extern const short kAvgDeltaOctaScore[];
  extern const uint32 kAvgDeltaOctaScoreSize;
  extern const uint32 kCompatTableIndSize;
  extern const uint32 kCjkDeltaBiIndSize;
  extern const uint32 kDistinctBiTableIndSize;
  extern const uint32 kQuadChromeIndSize;
  extern const uint32 kQuadChrome2IndSize;
  extern const uint32 kDeltaOctaIndSize;
  extern const uint32 kDistinctOctaIndSize;
}

int main(int argc, char** argv) {
  if (!CLD2DynamicData::isLittleEndian()) {
    fprintf(stderr, "System is big-endian: currently not supported.\n");
    return -1;
  }
  if (!CLD2DynamicData::coreAssumptionsOk()) {
    fprintf(stderr, "Core assumptions violated, unsafe to continue.\n");
    return -2;
  }

  // Get command-line flags
  int flags = 0;
  bool get_vector = false;
  char* fileName = NULL;
  const char* USAGE = "\
CLD2 Dynamic Data Tool:\n\
Dump, verify or print summaries of scoring tables for CLD2.\n\
\n\
The files output by this tool are suitable for all little-endian platforms,\n\
and should work on both 32- and 64-bit platforms.\n\
\n\
IMPORTANT: The files output by this tool WILL NOT work on big-endian platforms.\n\
\n\
Usage:\n\
  --dump [FILE]     Dump the scoring tables that this tool was linked against\n\
                    to the specified file. The tables are automatically verified\n\
                    after writing, just as if the tool was run again with\n\
                    '--verify'.\n\
  --verify [FILE]   Verify that a given file precisely matches the scoring\n\
                    tables that this tool was linked against. This can be used\n\
                    to verify that a file is compatible.\n\
  --head [FILE]     Print headers from the specified file to stdout.\n\
  --verbose         Be verbose.\n\
";
  int mode = 0; //1=dump, 2=verify, 3=head
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--verbose") == 0) {
      CLD2DynamicDataExtractor::setDebug(1);
      CLD2DynamicData::setDebug(1);
    }
    else if (strcmp(argv[i], "--dump") == 0
              || strcmp(argv[i], "--verify") == 0
              || strcmp(argv[i], "--head") == 0) {

      // set mode flag properly
      if (strcmp(argv[i], "--dump") == 0) mode=1;
      else if (strcmp(argv[i], "--verify") == 0) mode=2;
      else mode=3;
      if (i < argc - 1) {
        fileName = argv[++i];
      } else {
        fprintf(stderr, "Missing file name argument\n\n");
        fprintf(stderr, "%s", USAGE);
        return -1;
      }
    } else if (strcmp(argv[i], "--help") == 0) {
      fprintf(stdout, "%s", USAGE);
      return 0;
    } else {
      fprintf(stderr, "Unsupported option: %s\n\n", argv[i]);
      fprintf(stderr, "%s", USAGE);
      return -1;
    }
  }

  if (mode == 0) {
    fprintf(stderr, "%s", USAGE);
    return -1;
  }

  CLD2::ScoringTables realData = {
    &CLD2::cld_generated_CjkUni_obj,
    &CLD2::kCjkCompat_obj,
    &CLD2::kCjkDeltaBi_obj,
    &CLD2::kDistinctBiTable_obj,
    &CLD2::kQuad_obj,
    &CLD2::kQuad_obj2,
    &CLD2::kDeltaOcta_obj,
    &CLD2::kDistinctOcta_obj,
    CLD2::kAvgDeltaOctaScore,
  };
  const CLD2::uint32 indirectTableSizes[7] = {
    CLD2::kCompatTableIndSize,
    CLD2::kCjkDeltaBiIndSize,
    CLD2::kDistinctBiTableIndSize,
    CLD2::kQuadChromeIndSize,
    CLD2::kQuadChrome2IndSize,
    CLD2::kDeltaOctaIndSize,
    CLD2::kDistinctOctaIndSize
  };
  const CLD2DynamicData::Supplement supplement = {
    CLD2::kAvgDeltaOctaScoreSize,
    indirectTableSizes
  };
  if (mode == 1) { // dump
    CLD2DynamicDataExtractor::writeDataFile(
      static_cast<const CLD2::ScoringTables*>(&realData),
      &supplement,
      fileName);
  } else if (mode == 3) { // head
    CLD2DynamicData::FileHeader* header = CLD2DynamicDataLoader::loadHeaderFromFile(fileName);
    if (header == NULL) {
      fprintf(stderr, "Cannot read header from file: %s\n", fileName);
      return -1;
    }
    CLD2DynamicData::dumpHeader(header);
    delete[] header->tableHeaders;
    delete header;
  }
  
  if (mode == 1 || mode == 2) { // dump || verify (so perform verification)
    void* mmapAddress = NULL;
    uint32_t mmapLength = 0;
    CLD2::ScoringTables* loadedData = CLD2DynamicDataLoader::loadDataFile(fileName, &mmapAddress, &mmapLength);

    if (loadedData == NULL) {
      fprintf(stderr, "Failed to read data file: %s\n", fileName);
      return -1;
    }
    bool result = CLD2DynamicData::verify(
      static_cast<const CLD2::ScoringTables*>(&realData),
      &supplement,
      static_cast<const CLD2::ScoringTables*>(loadedData));
    CLD2DynamicDataLoader::unloadDataFile(&loadedData, &mmapAddress, &mmapLength);
    if (loadedData != NULL || mmapAddress != NULL || mmapLength != 0) {
      fprintf(stderr, "Warning: failed to clean up memory for ScoringTables.\n");
    }
    if (!result) {
      fprintf(stderr, "Verification failed!\n");
      return -1;
    }
  }
}
