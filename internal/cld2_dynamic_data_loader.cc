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
#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include "cld2_dynamic_data.h"
#include "cld2_dynamic_data_loader.h"
#include "integral_types.h"
#include "cld2tablesummary.h"
#include "utf8statetable.h"
#include "scoreonescriptspan.h"

namespace CLD2DynamicDataLoader {
static int DEBUG=0;

CLD2DynamicData::FileHeader* loadHeader(const char* fileName) {
  // TODO: force null-terminate char* strings for safety
  FILE* inFile = fopen(fileName, "r");
  if (inFile == NULL) {
    return NULL;
  }

  int bytesRead = 0;
  CLD2DynamicData::FileHeader* fileHeader = new CLD2DynamicData::FileHeader;
  bytesRead += fread(fileHeader->sanityString, 1, CLD2DynamicData::DATA_FILE_MARKER_LENGTH, inFile);
  if (!CLD2DynamicData::mem_compare(fileHeader->sanityString, CLD2DynamicData::DATA_FILE_MARKER, CLD2DynamicData::DATA_FILE_MARKER_LENGTH)) {
    std::cerr << "Malformed header: bad file marker!" << std::endl;
    delete fileHeader;
    return NULL;
  }

  bytesRead += 4 * fread(&(fileHeader->totalFileSizeBytes), 4, 1, inFile);
  bytesRead += 4 * fread(&(fileHeader->utf8PropObj_state0), 4, 1, inFile);
  bytesRead += 4 * fread(&(fileHeader->utf8PropObj_state0_size), 4, 1, inFile);
  bytesRead += 4 * fread(&(fileHeader->utf8PropObj_total_size), 4, 1, inFile);
  bytesRead += 4 * fread(&(fileHeader->utf8PropObj_max_expand), 4, 1, inFile);
  bytesRead += 4 * fread(&(fileHeader->utf8PropObj_entry_shift), 4, 1, inFile);
  bytesRead += 4 * fread(&(fileHeader->utf8PropObj_bytes_per_entry), 4, 1, inFile);
  bytesRead += 4 * fread(&(fileHeader->utf8PropObj_losub), 4, 1, inFile);
  bytesRead += 4 * fread(&(fileHeader->utf8PropObj_hiadd), 4, 1, inFile);
  bytesRead += 4 * fread(&(fileHeader->startOf_utf8PropObj_state_table), 4, 1, inFile);
  bytesRead += 4 * fread(&(fileHeader->lengthOf_utf8PropObj_state_table), 4, 1, inFile);
  bytesRead += 4 * fread(&(fileHeader->startOf_utf8PropObj_remap_base), 4, 1, inFile);
  bytesRead += 4 * fread(&(fileHeader->lengthOf_utf8PropObj_remap_base), 4, 1, inFile);
  bytesRead += 4 * fread(&(fileHeader->startOf_utf8PropObj_remap_string), 4, 1, inFile);
  bytesRead += 4 * fread(&(fileHeader->lengthOf_utf8PropObj_remap_string), 4, 1, inFile);
  bytesRead += 4 * fread(&(fileHeader->startOf_utf8PropObj_fast_state), 4, 1, inFile);
  bytesRead += 4 * fread(&(fileHeader->lengthOf_utf8PropObj_fast_state), 4, 1, inFile);
  bytesRead += 4 * fread(&(fileHeader->startOf_kAvgDeltaOctaScore), 4, 1, inFile);
  bytesRead += 4 * fread(&(fileHeader->lengthOf_kAvgDeltaOctaScore), 4, 1, inFile);
  bytesRead += 4 * fread(&(fileHeader->numTablesEncoded), 4, 1, inFile);

  CLD2DynamicData::TableHeader* tableHeaders = new CLD2DynamicData::TableHeader[fileHeader->numTablesEncoded];
  fileHeader->tableHeaders = tableHeaders;
  for (int x=0; x<fileHeader->numTablesEncoded; x++) {
    CLD2DynamicData::TableHeader &tHeader = fileHeader->tableHeaders[x];
    bytesRead += 4 * fread(&(tHeader.kCLDTableSizeOne), 4, 1, inFile);
    bytesRead += 4 * fread(&(tHeader.kCLDTableSize), 4, 1, inFile);
    bytesRead += 4 * fread(&(tHeader.kCLDTableKeyMask), 4, 1, inFile);
    bytesRead += 4 * fread(&(tHeader.kCLDTableBuildDate), 4, 1, inFile);
    bytesRead += 4 * fread(&(tHeader.startOf_kCLDTable), 4, 1, inFile);
    bytesRead += 4 * fread(&(tHeader.lengthOf_kCLDTable), 4, 1, inFile);
    bytesRead += 4 * fread(&(tHeader.startOf_kCLDTableInd), 4, 1, inFile);
    bytesRead += 4 * fread(&(tHeader.lengthOf_kCLDTableInd), 4, 1, inFile);
    bytesRead += 4 * fread(&(tHeader.startOf_kRecognizedLangScripts), 4, 1, inFile);
    bytesRead += 4 * fread(&(tHeader.lengthOf_kRecognizedLangScripts), 4, 1, inFile);
  }

  // Confirm header size is correct.
  int expectedHeaderSize = CLD2DynamicData::calculateHeaderSize(fileHeader->numTablesEncoded);
  if (expectedHeaderSize != bytesRead) {
    std::cerr << "Header size mismatch! Expected " << expectedHeaderSize << ", but read " << bytesRead << std::endl;
    delete fileHeader;
    delete tableHeaders;
    return NULL;
  }

  // Confirm file size is correct.
  fseek(inFile, 0, SEEK_END);
  int actualSize = ftell(inFile);
  fclose(inFile);

  if (actualSize != fileHeader->totalFileSizeBytes) {
    std::cerr << "File size mismatch! Expected " << fileHeader->totalFileSizeBytes << ", but found " << actualSize << std::endl;
    delete fileHeader;
    delete tableHeaders;
    return NULL;
  }
  return fileHeader;
}

void unloadData(CLD2::ScoringTables** scoringTables, void** mmapAddress, int* mmapLength) {
  free(const_cast<CLD2::UTF8PropObj*>((*scoringTables)->unigram_obj));
  (*scoringTables)->unigram_obj = NULL;
  delete((*scoringTables)->unigram_compat_obj); // tableSummaries[0] from loadDataFile
  (*scoringTables)->unigram_compat_obj = NULL;
  delete(*scoringTables);
  *scoringTables = NULL;
  munmap(*mmapAddress, *mmapLength);
  *mmapAddress = NULL;
  *mmapLength = 0;
}

CLD2::ScoringTables* loadDataFile(const char* fileName, void** mmapAddressOut, int* mmapLengthOut) {
  CLD2DynamicData::FileHeader* fileHeader = loadHeader(fileName);
  if (fileHeader == NULL) {
    return NULL;
  }

  // Initialize the memory map
  int inFileHandle = open(fileName, O_RDONLY);
  void* mapped = mmap(NULL, fileHeader->totalFileSizeBytes,
    PROT_READ, MAP_PRIVATE, inFileHandle, 0);
  // Record the map address. This allows callers to unmap 
  *mmapAddressOut=mapped;
  *mmapLengthOut=fileHeader->totalFileSizeBytes;
  close(inFileHandle);

  // 1. UTF8 Object
  const CLD2::uint8* state_table = static_cast<const CLD2::uint8*>(mapped) +
    fileHeader->startOf_utf8PropObj_state_table;
  // FIXME: Unsafe to rely on this since RemapEntry is not a bit-packed structure
  const CLD2::RemapEntry* remap_base =
    reinterpret_cast<const CLD2::RemapEntry*>(
      static_cast<const CLD2::uint8*>(mapped) +
      fileHeader->startOf_utf8PropObj_remap_base);
  const CLD2::uint8* remap_string = static_cast<const CLD2::uint8*>(mapped) +
    fileHeader->startOf_utf8PropObj_remap_string;
  const CLD2::uint8* fast_state =
    fileHeader->startOf_utf8PropObj_fast_state == 0 ? 0 :
      static_cast<const CLD2::uint8*>(mapped) +
      fileHeader->startOf_utf8PropObj_fast_state;

  // Populate intermediate object. Horrible casting required because the struct
  // is all read-only integers, and doesn't have a constructor. Yikes.
  // TODO: It might actually be less horrible to memcpy the data in <shudder>
  const CLD2::UTF8PropObj* unigram_obj = reinterpret_cast<CLD2::UTF8PropObj*>(malloc(sizeof(CLD2::UTF8PropObj)));
  *const_cast<CLD2::uint32*>(&unigram_obj->state0) = fileHeader->utf8PropObj_state0;
  *const_cast<CLD2::uint32*>(&unigram_obj->state0_size) = fileHeader->utf8PropObj_state0_size;
  *const_cast<CLD2::uint32*>(&unigram_obj->total_size) = fileHeader->utf8PropObj_total_size;
  *const_cast<int*>(&unigram_obj->max_expand) = fileHeader->utf8PropObj_max_expand;
  *const_cast<int*>(&unigram_obj->entry_shift) = fileHeader->utf8PropObj_entry_shift;
  *const_cast<int*>(&unigram_obj->bytes_per_entry) = fileHeader->utf8PropObj_bytes_per_entry;
  *const_cast<CLD2::uint32*>(&unigram_obj->losub) = fileHeader->utf8PropObj_losub;
  *const_cast<CLD2::uint32*>(&unigram_obj->hiadd) = fileHeader->utf8PropObj_hiadd;
  *const_cast<const CLD2::uint8**>(&unigram_obj->state_table) = state_table;
  *const_cast<const CLD2::RemapEntry**>(&unigram_obj->remap_base) = remap_base;
  *const_cast<const CLD2::uint8**>(&unigram_obj->remap_string) = remap_string;
  *const_cast<const CLD2::uint8**>(&unigram_obj->fast_state) = fast_state;

  // 2. kAvgDeltaOctaScore array
  const short* read_kAvgDeltaOctaScore = reinterpret_cast<const short*>(
    static_cast<const CLD2::uint8*>(mapped) +
    fileHeader->startOf_kAvgDeltaOctaScore);

  // 3. Each table
  CLD2::CLD2TableSummary* tableSummaries = new CLD2::CLD2TableSummary[fileHeader->numTablesEncoded];
  for (int x=0; x<fileHeader->numTablesEncoded; x++) {
    CLD2::CLD2TableSummary &summary = tableSummaries[x];
    CLD2DynamicData::TableHeader& tHeader = fileHeader->tableHeaders[x];
    const CLD2::IndirectProbBucket4* kCLDTable =
      reinterpret_cast<const CLD2::IndirectProbBucket4*>(
        static_cast<CLD2::uint8*>(mapped) + tHeader.startOf_kCLDTable);
    const CLD2::uint32* kCLDTableInd =
      reinterpret_cast<const CLD2::uint32*>(
        static_cast<CLD2::uint8*>(mapped) + tHeader.startOf_kCLDTableInd);
    const char* kRecognizedLangScripts =
      static_cast<const char*>(mapped) + tHeader.startOf_kRecognizedLangScripts;

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
  delete fileHeader->tableHeaders;
  delete fileHeader;
  return result;
}
}
