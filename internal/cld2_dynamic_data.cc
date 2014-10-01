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

#include "cld2_dynamic_data.h"
#include "integral_types.h"
#include <assert.h>
#include <stdint.h>

namespace CLD2DynamicData {
static int DEBUG=0;
void setDebug(int debug) {
  DEBUG=debug;
}

bool mem_compare(const void* data1, const void* data2, const int length) {
  const unsigned char* raw1 = static_cast<const unsigned char*>(data1);
  const unsigned char* raw2 = static_cast<const unsigned char*>(data2);
  for (int x=0; x<length; x++) {
    if (raw1[x] != raw2[x]) {
      fprintf(stderr, "mem difference at data[%d]: decimal %d != decimal %d\n",
              x, (unsigned int) raw1[x], (unsigned int) raw2[x]);
      int y = (x - 5 > 0) ? (x - 5) : 0; // https://code.google.com/p/cld2/issues/detail?id=24
      for (; y<length && y<=x+5; y++) {
        fprintf(stderr, "[%d]: %d <-> %d%s\n",
                y, (unsigned int) raw1[y], (unsigned int) raw2[y],
                ( x == y ? " [FIRST ERROR DETECTED HERE] " : ""));
      }
      return false;
    }
  }
  return true;
}

CLD2::uint32 calculateHeaderSize(CLD2::uint32 numTables) {
  return DATA_FILE_MARKER_LENGTH // NB: no null terminator
    + (20 * sizeof(CLD2::uint32)) // 20 uint32 fields in the struct
    + (numTables * (10 * sizeof(CLD2::uint32))); // 10 uint32 per table
}

void dumpHeader(FileHeader* header) {
  char safeString[DATA_FILE_MARKER_LENGTH + 1];
  memcpy(safeString, header->sanityString, DATA_FILE_MARKER_LENGTH);
  safeString[DATA_FILE_MARKER_LENGTH] = 0;
  fprintf(stdout, "sanityString: %s\n", safeString);
  fprintf(stdout, "totalFileSizeBytes: %d\n", header->totalFileSizeBytes);
  fprintf(stdout, "utf8PropObj_state0: %d\n", header->utf8PropObj_state0);
  fprintf(stdout, "utf8PropObj_state0_size: %d\n", header->utf8PropObj_state0_size);
  fprintf(stdout, "utf8PropObj_total_size: %d\n", header->utf8PropObj_total_size);
  fprintf(stdout, "utf8PropObj_max_expand: %d\n", header->utf8PropObj_max_expand);
  fprintf(stdout, "utf8PropObj_entry_shift: %d\n", header->utf8PropObj_entry_shift);
  fprintf(stdout, "utf8PropObj_bytes_per_entry: %d\n", header->utf8PropObj_bytes_per_entry);
  fprintf(stdout, "utf8PropObj_losub: %d\n", header->utf8PropObj_losub);
  fprintf(stdout, "utf8PropObj_hiadd: %d\n", header->utf8PropObj_hiadd);
  fprintf(stdout, "startOf_utf8PropObj_state_table: %d\n", header->startOf_utf8PropObj_state_table);
  fprintf(stdout, "lengthOf_utf8PropObj_state_table: %d\n", header->lengthOf_utf8PropObj_state_table);
  fprintf(stdout, "startOf_utf8PropObj_remap_base: %d\n", header->startOf_utf8PropObj_remap_base);
  fprintf(stdout, "lengthOf_utf8PropObj_remap_base: %d\n", header->lengthOf_utf8PropObj_remap_base);
  fprintf(stdout, "startOf_utf8PropObj_remap_string: %d\n", header->startOf_utf8PropObj_remap_string);
  fprintf(stdout, "lengthOf_utf8PropObj_remap_string: %d\n", header->lengthOf_utf8PropObj_remap_string);
  fprintf(stdout, "startOf_utf8PropObj_fast_state: %d\n", header->startOf_utf8PropObj_fast_state);
  fprintf(stdout, "lengthOf_utf8PropObj_fast_state: %d\n", header->lengthOf_utf8PropObj_fast_state);
  fprintf(stdout, "startOf_kAvgDeltaOctaScore: %d\n", header->startOf_kAvgDeltaOctaScore);
  fprintf(stdout, "lengthOf_kAvgDeltaOctaScore: %d\n", header->lengthOf_kAvgDeltaOctaScore);
  fprintf(stdout, "numTablesEncoded: %d\n", header->numTablesEncoded);

  const char* tableNames[7];
  tableNames[0]="unigram_compat_obj";
  tableNames[1]="deltabi_obj";
  tableNames[2]="distinctbi_obj";
  tableNames[3]="quadgram_obj";
  tableNames[4]="quadgram_obj2";
  tableNames[5]="deltaocta_obj";
  tableNames[6]="distinctocta_obj";

  for (int x=0; x < (int) header->numTablesEncoded; x++) {
    TableHeader& tHeader = header->tableHeaders[x];
      
    fprintf(stdout, "Table %d: (%s)\n", (x+1), tableNames[x]);;
    fprintf(stdout, "  kCLDTableSizeOne: %d\n", tHeader.kCLDTableSizeOne);
    fprintf(stdout, "  kCLDTableSize: %d\n", tHeader.kCLDTableSize);
    fprintf(stdout, "  kCLDTableKeyMask: %d\n", tHeader.kCLDTableKeyMask);
    fprintf(stdout, "  kCLDTableBuildDate: %d\n", tHeader.kCLDTableBuildDate);
    fprintf(stdout, "  startOf_kCLDTable: %d\n", tHeader.startOf_kCLDTable);
    fprintf(stdout, "  lengthOf_kCLDTable: %d\n", tHeader.lengthOf_kCLDTable);
    fprintf(stdout, "  startOf_kCLDTableInd: %d\n", tHeader.startOf_kCLDTableInd);
    fprintf(stdout, "  lengthOf_kCLDTableInd: %d\n", tHeader.lengthOf_kCLDTableInd);
    fprintf(stdout, "  startOf_kRecognizedLangScripts: %d\n", tHeader.startOf_kRecognizedLangScripts);
    fprintf(stdout, "  lengthOf_kRecognizedLangScripts: %d\n", tHeader.lengthOf_kRecognizedLangScripts);
  }
}

#define CHECK_EQUALS(name) if (loadedData->name != realData->name) {\
    fprintf(stderr, "%s: %d != %d\n", #name, loadedData->name, realData->name);\
  return false;\
}

#define CHECK_MEM_EQUALS(name,size) if (!mem_compare(loadedData->name,realData->name,size)) {\
  fprintf(stderr, "%s: data mismatch.\n", #name);\
  return false;\
}

bool verify(const CLD2::ScoringTables* realData,
            const Supplement* realSupplement,
            const CLD2::ScoringTables* loadedData) {
  const int NUM_TABLES = 7;
  const CLD2::CLD2TableSummary* realTableSummaries[NUM_TABLES];
  realTableSummaries[0] = realData->unigram_compat_obj;
  realTableSummaries[1] = realData->deltabi_obj;
  realTableSummaries[2] = realData->distinctbi_obj;
  realTableSummaries[3] = realData->quadgram_obj;
  realTableSummaries[4] = realData->quadgram_obj2;
  realTableSummaries[5] = realData->deltaocta_obj;
  realTableSummaries[6] = realData->distinctocta_obj;

  const CLD2::CLD2TableSummary* loadedTableSummaries[NUM_TABLES];
  loadedTableSummaries[0] = loadedData->unigram_compat_obj;
  loadedTableSummaries[1] = loadedData->deltabi_obj;
  loadedTableSummaries[2] = loadedData->distinctbi_obj;
  loadedTableSummaries[3] = loadedData->quadgram_obj;
  loadedTableSummaries[4] = loadedData->quadgram_obj2;
  loadedTableSummaries[5] = loadedData->deltaocta_obj;
  loadedTableSummaries[6] = loadedData->distinctocta_obj;

  CHECK_EQUALS(unigram_obj->state0);
  CHECK_EQUALS(unigram_obj->state0_size);
  CHECK_EQUALS(unigram_obj->total_size);
  CHECK_EQUALS(unigram_obj->max_expand);
  CHECK_EQUALS(unigram_obj->entry_shift);
  CHECK_EQUALS(unigram_obj->bytes_per_entry);
  CHECK_EQUALS(unigram_obj->losub);
  CHECK_EQUALS(unigram_obj->hiadd);
  CHECK_MEM_EQUALS(unigram_obj->state_table, realData->unigram_obj->total_size);
  CHECK_MEM_EQUALS(unigram_obj->remap_base, sizeof(CLD2::RemapEntry)); // TODO: can this have more than one entry?
  CHECK_MEM_EQUALS(unigram_obj->remap_string, strlen(
      reinterpret_cast<const char*>(realData->unigram_obj->remap_string)) + 1); // null terminator included

  if (loadedData->unigram_obj->fast_state == NULL) {
    if (realData->unigram_obj->fast_state != NULL) {
      fprintf(stderr, "unigram_obj->fast_state is missing.\n");
      return false;
    }
  } else {
    if (realData->unigram_obj->fast_state == NULL) {
      fprintf(stderr, "unigram_obj->fast_state shouldn't be present.\n");
      return false;
    }
    CHECK_MEM_EQUALS(unigram_obj->fast_state, strlen(
      reinterpret_cast<const char*>(realData->unigram_obj->fast_state)) + 1); // null terminator included
  }
  if (DEBUG) fprintf(stdout, "verified.\n");

  if (DEBUG) fprintf(stdout, "Verifying kExpectedScore... ");
  CHECK_MEM_EQUALS(kExpectedScore, realSupplement->lengthOf_kAvgDeltaOctaScore);
  if (DEBUG) fprintf(stdout, "verified.\n");

  // 3. Each table
  for (int x=0; x<NUM_TABLES; x++) {
    if (DEBUG) fprintf(stdout, "Verifying table %d... ", x+1);
    const CLD2::CLD2TableSummary* realData = realTableSummaries[x];
    const CLD2::CLD2TableSummary* loadedData = loadedTableSummaries[x];
    // We need to calculate the table lengths to do the memcmp
    CLD2::uint32 bytesPerBucket = sizeof(CLD2::IndirectProbBucket4);
    CLD2::uint32 numBuckets = realData->kCLDTableSize;
    CLD2::uint32 tableSizeBytes = bytesPerBucket * numBuckets;
    CLD2::uint32 indirectTableSizeBytes = realSupplement->indirectTableSizes[x];
    CLD2::uint32 recognizedScriptsSizeBytes =
      strlen(realData->kRecognizedLangScripts) + 1; // null terminator included

    // Verify the table data
    CHECK_EQUALS(kCLDTableSizeOne);
    CHECK_EQUALS(kCLDTableSize);
    CHECK_EQUALS(kCLDTableKeyMask);
    CHECK_EQUALS(kCLDTableBuildDate);
    CHECK_MEM_EQUALS(kCLDTable, tableSizeBytes);
    CHECK_MEM_EQUALS(kCLDTableInd, indirectTableSizeBytes);
    CHECK_MEM_EQUALS(kRecognizedLangScripts, recognizedScriptsSizeBytes);
    if (DEBUG) fprintf(stdout, "verified.\n");
  }
  if (DEBUG) fprintf(stdout, "All data verified successfully.\n");
  return true;
}

// As noted on http://stackoverflow.com/questions/1001307, gcc is highly likely
// to convert this function's return into a constant - meaning that any
// if-branches based upon it will be eliminated at compile time, allowing
// "free" detection throughout any dependent code.
bool isLittleEndian() {
  union {
    uint32_t integer;
    char bytes[4];
  } test = {0x01020304};
  return test.bytes[0] == 4;
}

bool coreAssumptionsOk() {
  if (sizeof(CLD2::uint8) != 1) {
    fprintf(stderr, "uint8 is %d bits instead of 8!\n", (int) (sizeof(CLD2::uint8) * 8));
    return false;
  }
  if (sizeof(CLD2::uint16) != 2) {
    fprintf(stderr, "uint16 is %d bits instead of 16!\n", (int) (sizeof(CLD2::uint16) * 8));
    return false;
  }
  if (sizeof(CLD2::uint32) != 4) {
    fprintf(stderr, "uint32 is %d bits instead of 32!\n", (int) (sizeof(CLD2::uint32) * 8));
    return false;
  }
  return true;
}

} // End namespace CLD2DynamicData
