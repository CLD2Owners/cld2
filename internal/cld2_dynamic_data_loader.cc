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
#include <stdint.h>
#include <stdio.h>
#include <fstream>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include "cld2_dynamic_compat.h" // for win32/posix compatibility
#include "cld2_dynamic_data.h"
#include "cld2_dynamic_data_loader.h"
#include "integral_types.h"
#include "cld2tablesummary.h"
#include "utf8statetable.h"
#include "scoreonescriptspan.h"

namespace CLD2DynamicDataLoader {
static int DEBUG=0;

CLD2DynamicData::FileHeader* loadHeaderFromFile(const char* fileName) {
  FILE* inFile = fopen(fileName, "r");
  if (inFile == NULL) {
    return NULL;
  }
  return loadInternal(inFile, NULL, -1);
}

CLD2DynamicData::FileHeader* loadHeaderFromRaw(const void* basePointer,
                                               const uint32_t length) {
  return loadInternal(NULL, basePointer, length);
}


#define CLD2_READINT(field) \
  if (sourceIsFile) {\
    bytesRead += 4 * fread(&(header->field), 4, 1, inFile);\
  } else {\
    memcpy(&(header->field), (((char*)(basePointer)) + bytesRead), 4);\
    bytesRead += 4;\
  }
CLD2DynamicData::FileHeader* loadInternal(FILE* inFile, const void* basePointer, const uint32_t length) {
  const bool sourceIsFile = (inFile != NULL);
  int bytesRead = 0;
  CLD2DynamicData::FileHeader* header = new CLD2DynamicData::FileHeader;

  // TODO: force null-terminate char* strings for safety
  if (sourceIsFile) {
    bytesRead += fread(header->sanityString, 1, CLD2DynamicData::DATA_FILE_MARKER_LENGTH, inFile);
  } else {
    memcpy(header->sanityString, basePointer, CLD2DynamicData::DATA_FILE_MARKER_LENGTH);
    bytesRead += CLD2DynamicData::DATA_FILE_MARKER_LENGTH;
  }

  if (!CLD2DynamicData::mem_compare(
                                    header->sanityString,
                                    CLD2DynamicData::DATA_FILE_MARKER,
                                    CLD2DynamicData::DATA_FILE_MARKER_LENGTH)) {
    fprintf(stderr, "Malformed header: bad file marker!\n");
    delete header;
    return NULL;
  }

  CLD2_READINT(totalFileSizeBytes);
  CLD2_READINT(utf8PropObj_state0);
  CLD2_READINT(utf8PropObj_state0_size);
  CLD2_READINT(utf8PropObj_total_size);
  CLD2_READINT(utf8PropObj_max_expand);
  CLD2_READINT(utf8PropObj_entry_shift);
  CLD2_READINT(utf8PropObj_bytes_per_entry);
  CLD2_READINT(utf8PropObj_losub);
  CLD2_READINT(utf8PropObj_hiadd);
  CLD2_READINT(startOf_utf8PropObj_state_table);
  CLD2_READINT(lengthOf_utf8PropObj_state_table);
  CLD2_READINT(startOf_utf8PropObj_remap_base);
  CLD2_READINT(lengthOf_utf8PropObj_remap_base);
  CLD2_READINT(startOf_utf8PropObj_remap_string);
  CLD2_READINT(lengthOf_utf8PropObj_remap_string);
  CLD2_READINT(startOf_utf8PropObj_fast_state);
  CLD2_READINT(lengthOf_utf8PropObj_fast_state);
  CLD2_READINT(startOf_kAvgDeltaOctaScore);
  CLD2_READINT(lengthOf_kAvgDeltaOctaScore);
  CLD2_READINT(numTablesEncoded);

  CLD2DynamicData::TableHeader* tableHeaders = new CLD2DynamicData::TableHeader[header->numTablesEncoded];
  header->tableHeaders = tableHeaders;
  for (int x=0; x < (int) header->numTablesEncoded; x++) {
    CLD2DynamicData::TableHeader *header = &(tableHeaders[x]);
    CLD2_READINT(kCLDTableSizeOne);
    CLD2_READINT(kCLDTableSize);
    CLD2_READINT(kCLDTableKeyMask);
    CLD2_READINT(kCLDTableBuildDate);
    CLD2_READINT(startOf_kCLDTable);
    CLD2_READINT(lengthOf_kCLDTable);
    CLD2_READINT(startOf_kCLDTableInd);
    CLD2_READINT(lengthOf_kCLDTableInd);
    CLD2_READINT(startOf_kRecognizedLangScripts);
    CLD2_READINT(lengthOf_kRecognizedLangScripts);
  }

  // Confirm header size is correct.
  int expectedHeaderSize = CLD2DynamicData::calculateHeaderSize(header->numTablesEncoded);
  if (expectedHeaderSize != bytesRead) {
    fprintf(stderr, "Header size mismatch! Expected %d, but read %d\n", expectedHeaderSize, bytesRead);
    delete header;
    delete[] tableHeaders;
    return NULL;
  }

  int actualSize = 0;
  if (sourceIsFile) {
    // Confirm file size is correct.
    fseek(inFile, 0, SEEK_END);
    actualSize = ftell(inFile);
    fclose(inFile);
  } else {
    actualSize = length;
  }

  if (actualSize != header->totalFileSizeBytes) {
    fprintf(stderr, "File size mismatch! Expected %d, but found %d\n", header->totalFileSizeBytes, actualSize);
    delete header;
    delete[] tableHeaders;
    return NULL;
  }
  return header;
}

void unloadDataFile(CLD2::ScoringTables** scoringTables,
                    void** mmapAddress, uint32_t* mmapLength) {
#ifdef _WIN32
  // See https://code.google.com/p/cld2/issues/detail?id=20
  fprintf(stderr, "dynamic data unloading from file is not currently supported on win32, use raw mode instead.");
  return;
#else // i.e., is POSIX (no support for Mac prior to OSX)
  CLD2DynamicDataLoader::unloadDataRaw(scoringTables);
  munmap(*mmapAddress, *mmapLength);
  *mmapAddress = NULL;
  *mmapLength = 0;
#endif // ifdef _WIN32
}

void unloadDataRaw(CLD2::ScoringTables** scoringTables) {
  free(const_cast<CLD2::UTF8PropObj*>((*scoringTables)->unigram_obj));
  (*scoringTables)->unigram_obj = NULL;
  delete((*scoringTables)->unigram_compat_obj); // tableSummaries[0] from loadDataFile
  (*scoringTables)->unigram_compat_obj = NULL;
  delete(*scoringTables);
  *scoringTables = NULL;
}

CLD2::ScoringTables* loadDataFile(const char* fileName,
                                  void** mmapAddressOut, uint32_t* mmapLengthOut) {

#ifdef _WIN32
  // See https://code.google.com/p/cld2/issues/detail?id=20
  fprintf(stderr, "dynamic data loading from file is not currently supported on win32, use raw mode instead.");
  return NULL;
#else // i.e., is POSIX (no support for Mac prior to OSX)
  CLD2DynamicData::FileHeader* header = loadHeaderFromFile(fileName);
  if (header == NULL) {
    return NULL;
  }

  // Initialize the memory map
  int inFileHandle = OPEN(fileName, O_RDONLY);
  void* mapped = mmap(NULL, header->totalFileSizeBytes,
    PROT_READ, MAP_PRIVATE, inFileHandle, 0);
  // Record the map address. This allows callers to unmap 
  *mmapAddressOut=mapped;
  *mmapLengthOut=header->totalFileSizeBytes;
  CLOSE(inFileHandle);

  return loadDataInternal(header, mapped, header->totalFileSizeBytes);
#endif // ifdef _WIN32
}

CLD2::ScoringTables* loadDataRaw(const void* basePointer, const uint32_t length) {
  CLD2DynamicData::FileHeader* header = loadHeaderFromRaw(basePointer, length);
  return loadDataInternal(header, basePointer, length);
}

CLD2::ScoringTables* loadDataInternal(CLD2DynamicData::FileHeader* header, const void* basePointer, const uint32_t length) {
  // 1. UTF8 Object
  const CLD2::uint8* state_table = static_cast<const CLD2::uint8*>(basePointer) +
    header->startOf_utf8PropObj_state_table;
  // FIXME: Unsafe to rely on this since RemapEntry is not a bit-packed structure
  const CLD2::RemapEntry* remap_base =
    reinterpret_cast<const CLD2::RemapEntry*>(
      static_cast<const CLD2::uint8*>(basePointer) +
      header->startOf_utf8PropObj_remap_base);
  const CLD2::uint8* remap_string = static_cast<const CLD2::uint8*>(basePointer) +
    header->startOf_utf8PropObj_remap_string;
  const CLD2::uint8* fast_state =
    header->startOf_utf8PropObj_fast_state == 0 ? 0 :
      static_cast<const CLD2::uint8*>(basePointer) +
      header->startOf_utf8PropObj_fast_state;

  // Populate intermediate object. Horrible casting required because the struct
  // is all read-only integers, and doesn't have a constructor. Yikes.
  // TODO: It might actually be less horrible to memcpy the data in <shudder>
  const CLD2::UTF8PropObj* unigram_obj = reinterpret_cast<CLD2::UTF8PropObj*>(malloc(sizeof(CLD2::UTF8PropObj)));
  *const_cast<CLD2::uint32*>(&unigram_obj->state0) = header->utf8PropObj_state0;
  *const_cast<CLD2::uint32*>(&unigram_obj->state0_size) = header->utf8PropObj_state0_size;
  *const_cast<CLD2::uint32*>(&unigram_obj->total_size) = header->utf8PropObj_total_size;
  *const_cast<int*>(&unigram_obj->max_expand) = header->utf8PropObj_max_expand;
  *const_cast<int*>(&unigram_obj->entry_shift) = header->utf8PropObj_entry_shift;
  *const_cast<int*>(&unigram_obj->bytes_per_entry) = header->utf8PropObj_bytes_per_entry;
  *const_cast<CLD2::uint32*>(&unigram_obj->losub) = header->utf8PropObj_losub;
  *const_cast<CLD2::uint32*>(&unigram_obj->hiadd) = header->utf8PropObj_hiadd;
  *const_cast<const CLD2::uint8**>(&unigram_obj->state_table) = state_table;
  *const_cast<const CLD2::RemapEntry**>(&unigram_obj->remap_base) = remap_base;
  *const_cast<const CLD2::uint8**>(&unigram_obj->remap_string) = remap_string;
  *const_cast<const CLD2::uint8**>(&unigram_obj->fast_state) = fast_state;

  // 2. kAvgDeltaOctaScore array
  const short* read_kAvgDeltaOctaScore = reinterpret_cast<const short*>(
    static_cast<const CLD2::uint8*>(basePointer) +
    header->startOf_kAvgDeltaOctaScore);

  // 3. Each table
  CLD2::CLD2TableSummary* tableSummaries = new CLD2::CLD2TableSummary[header->numTablesEncoded];
  for (int x=0; x < (int) header->numTablesEncoded; x++) {
    CLD2::CLD2TableSummary &summary = tableSummaries[x];
    CLD2DynamicData::TableHeader& tHeader = header->tableHeaders[x];
    const CLD2::IndirectProbBucket4* kCLDTable =
      reinterpret_cast<const CLD2::IndirectProbBucket4*>(
        static_cast<const CLD2::uint8*>(basePointer) + tHeader.startOf_kCLDTable);
    const CLD2::uint32* kCLDTableInd =
      reinterpret_cast<const CLD2::uint32*>(
        static_cast<const CLD2::uint8*>(basePointer) + tHeader.startOf_kCLDTableInd);
    const char* kRecognizedLangScripts =
      static_cast<const char*>(basePointer) + tHeader.startOf_kRecognizedLangScripts;

    summary.kCLDTable = kCLDTable;
    summary.kCLDTableInd = kCLDTableInd;
    summary.kCLDTableSizeOne = tHeader.kCLDTableSizeOne;
    summary.kCLDTableSize = tHeader.kCLDTableSize;
    summary.kCLDTableKeyMask = tHeader.kCLDTableKeyMask;
    summary.kCLDTableBuildDate = tHeader.kCLDTableBuildDate;
    summary.kRecognizedLangScripts = kRecognizedLangScripts;
  }

  // Tie everything together
  CLD2::ScoringTables* result = new CLD2::ScoringTables;
  result->unigram_obj = unigram_obj;
  result->unigram_compat_obj = &tableSummaries[0];
  result->deltabi_obj = &tableSummaries[1];
  result->distinctbi_obj = &tableSummaries[2];
  result->quadgram_obj = &tableSummaries[3];
  result->quadgram_obj2 = &tableSummaries[4];
  result->deltaocta_obj = &tableSummaries[5];
  result->distinctocta_obj = &tableSummaries[6];
  result->kExpectedScore = read_kAvgDeltaOctaScore;
  delete[] header->tableHeaders;
  delete header;
  return result;
}

} // namespace CLD2DynamicDataLoader
