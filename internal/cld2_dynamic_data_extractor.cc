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
#include "cld2_dynamic_data_extractor.h"
#include "cld2_dynamic_data_loader.h" // for verifying the written data
#include "integral_types.h"
#include "cld2tablesummary.h"
#include "utf8statetable.h"

using namespace std;
namespace CLD2DynamicDataExtractor {
static int DEBUG=0;
void setDebug(int debug) {
  DEBUG=debug;
}

int advance(FILE* f, CLD2::uint32 position) {
  const char ZERO = 0;
  int pad = position - ftell(f);
  if (DEBUG) fprintf(stdout, "  Adding %d bytes of padding\n", pad);
  while (pad-- > 0) {
    fwrite(&ZERO,1,1,f);
  }
  return pad;
}

void writeChunk(FILE *f, const void* data, CLD2::uint32 startAt, CLD2::uint32 length) {
  if (DEBUG) fprintf(stdout, "  Write chunk @%d, len=%d\n", startAt, length);
  advance(f, startAt);
  if (DEBUG) fprintf(stdout, "  Writing %d bytes of data", length);;
  fwrite(data, 1, length, f);
}

void writeDataFile(const CLD2::ScoringTables* data,
                   const CLD2DynamicData::Supplement* supplement,
                   const char* fileName) {
  // The order here is hardcoded and MUST NOT BE CHANGED, else you will de-sync
  // with the reading code.
  const char ZERO = 0;
  const int NUM_TABLES = 7;
  const CLD2::CLD2TableSummary* tableSummaries[NUM_TABLES];
  tableSummaries[0] = data->unigram_compat_obj;
  tableSummaries[1] = data->deltabi_obj;
  tableSummaries[2] = data->distinctbi_obj;
  tableSummaries[3] = data->quadgram_obj;
  tableSummaries[4] = data->quadgram_obj2;
  tableSummaries[5] = data->deltaocta_obj;
  tableSummaries[6] = data->distinctocta_obj;

  CLD2DynamicData::TableHeader tableHeaders[NUM_TABLES];
  CLD2DynamicData::FileHeader fileHeader;
  fileHeader.numTablesEncoded = NUM_TABLES;
  fileHeader.tableHeaders = tableHeaders;
  initUtf8Headers(&fileHeader, data->unigram_obj);
  initDeltaHeaders(&fileHeader, supplement->lengthOf_kAvgDeltaOctaScore);
  initTableHeaders(tableSummaries, NUM_TABLES, supplement, tableHeaders);
  alignAll(&fileHeader, 16); // Align all sections to 128-bit boundaries

  // We are ready to rock.
  for (int x=0; x<CLD2DynamicData::DATA_FILE_MARKER_LENGTH; x++)
    fileHeader.sanityString[x] = CLD2DynamicData::DATA_FILE_MARKER[x];
  FILE* outFile = fopen(fileName, "w");
  fwrite(fileHeader.sanityString, 1, CLD2DynamicData::DATA_FILE_MARKER_LENGTH, outFile);
  fwrite(&(fileHeader.totalFileSizeBytes), 4, 1, outFile);
  fwrite(&(fileHeader.utf8PropObj_state0), 4, 1, outFile);
  fwrite(&(fileHeader.utf8PropObj_state0_size), 4, 1, outFile);
  fwrite(&(fileHeader.utf8PropObj_total_size), 4, 1, outFile);
  fwrite(&(fileHeader.utf8PropObj_max_expand), 4, 1, outFile);
  fwrite(&(fileHeader.utf8PropObj_entry_shift), 4, 1, outFile);
  fwrite(&(fileHeader.utf8PropObj_bytes_per_entry), 4, 1, outFile);
  fwrite(&(fileHeader.utf8PropObj_losub), 4, 1, outFile);
  fwrite(&(fileHeader.utf8PropObj_hiadd), 4, 1, outFile);
  fwrite(&(fileHeader.startOf_utf8PropObj_state_table), 4, 1, outFile);
  fwrite(&(fileHeader.lengthOf_utf8PropObj_state_table), 4, 1, outFile);
  fwrite(&(fileHeader.startOf_utf8PropObj_remap_base), 4, 1, outFile);
  fwrite(&(fileHeader.lengthOf_utf8PropObj_remap_base), 4, 1, outFile);
  fwrite(&(fileHeader.startOf_utf8PropObj_remap_string), 4, 1, outFile);
  fwrite(&(fileHeader.lengthOf_utf8PropObj_remap_string), 4, 1, outFile);
  fwrite(&(fileHeader.startOf_utf8PropObj_fast_state), 4, 1, outFile);
  fwrite(&(fileHeader.lengthOf_utf8PropObj_fast_state), 4, 1, outFile);
  fwrite(&(fileHeader.startOf_kAvgDeltaOctaScore), 4, 1, outFile);
  fwrite(&(fileHeader.lengthOf_kAvgDeltaOctaScore), 4, 1, outFile);
  fwrite(&(fileHeader.numTablesEncoded), 4, 1, outFile);
  for (int x=0; x<NUM_TABLES; x++) {
    CLD2DynamicData::TableHeader& tHeader = fileHeader.tableHeaders[x];
    fwrite(&(tHeader.kCLDTableSizeOne), 4, 1, outFile);
    fwrite(&(tHeader.kCLDTableSize), 4, 1, outFile);
    fwrite(&(tHeader.kCLDTableKeyMask), 4, 1, outFile);
    fwrite(&(tHeader.kCLDTableBuildDate), 4, 1, outFile);
    fwrite(&(tHeader.startOf_kCLDTable), 4, 1, outFile);
    fwrite(&(tHeader.lengthOf_kCLDTable), 4, 1, outFile);
    fwrite(&(tHeader.startOf_kCLDTableInd), 4, 1, outFile);
    fwrite(&(tHeader.lengthOf_kCLDTableInd), 4, 1, outFile);
    fwrite(&(tHeader.startOf_kRecognizedLangScripts), 4, 1, outFile);
    fwrite(&(tHeader.lengthOf_kRecognizedLangScripts), 4, 1, outFile);
  }

  // Write data blob
  // 1. UTF8 Object
  writeChunk(outFile,
    data->unigram_obj->state_table,
    fileHeader.startOf_utf8PropObj_state_table,
    fileHeader.lengthOf_utf8PropObj_state_table);
  // FIXME: Unsafe to rely on this since RemapEntry is not a bit-packed structure
  writeChunk(outFile,
    data->unigram_obj->remap_base,
    fileHeader.startOf_utf8PropObj_remap_base,
    fileHeader.lengthOf_utf8PropObj_remap_base);
  writeChunk(outFile,
    data->unigram_obj->remap_string,
    fileHeader.startOf_utf8PropObj_remap_string,
    fileHeader.lengthOf_utf8PropObj_remap_string - 1);
  fwrite(&ZERO,1,1,outFile); // null terminator
  if (fileHeader.startOf_utf8PropObj_fast_state > 0) {
    writeChunk(outFile,
      data->unigram_obj->fast_state,
      fileHeader.startOf_utf8PropObj_fast_state,
      fileHeader.lengthOf_utf8PropObj_fast_state - 1);
    fwrite(&ZERO,1,1,outFile); // null terminator
  }

  // 2. kAvgDeltaOctaScore array
  writeChunk(outFile,
    data->kExpectedScore,
    fileHeader.startOf_kAvgDeltaOctaScore,
    fileHeader.lengthOf_kAvgDeltaOctaScore);

  // 3. Each table
  for (int x=0; x<NUM_TABLES; x++) {
    const CLD2::CLD2TableSummary* summary = tableSummaries[x];
    CLD2DynamicData::TableHeader& tHeader = fileHeader.tableHeaders[x];
    // NB: Safe to directly write IndirectProbBucket4 as it is just an alias for CLD2::uint32
    writeChunk(outFile,
      summary->kCLDTable,
      tHeader.startOf_kCLDTable,
      tHeader.lengthOf_kCLDTable);
    writeChunk(outFile,
      summary->kCLDTableInd,
      tHeader.startOf_kCLDTableInd,
      tHeader.lengthOf_kCLDTableInd);
    writeChunk(outFile,
      summary->kRecognizedLangScripts,
      tHeader.startOf_kRecognizedLangScripts,
      tHeader.lengthOf_kRecognizedLangScripts - 1);
    fwrite(&ZERO,1,1,outFile); // null terminator
  }
  fclose(outFile);
}

void initTableHeaders(const CLD2::CLD2TableSummary** summaries,
                      const int numSummaries,
                      const CLD2DynamicData::Supplement* supplement,
                      CLD2DynamicData::TableHeader* tableHeaders) {
  // Important: As documented in the .h, we assume that the Supplement data
  // structure contains exactly one entry in indirectTableSizes for each
  // CLD2TableSummary, in the same order.
  for (int x=0; x<numSummaries; x++) {
    const CLD2::CLD2TableSummary* summary = summaries[x];
    CLD2DynamicData::TableHeader& tableHeader = tableHeaders[x];

    // Copy the primitive bits
    tableHeader.kCLDTableSizeOne = summary->kCLDTableSizeOne;
    tableHeader.kCLDTableSize = summary->kCLDTableSize;
    tableHeader.kCLDTableKeyMask = summary->kCLDTableKeyMask;
    tableHeader.kCLDTableBuildDate = summary->kCLDTableBuildDate;

    // Calculate size information
    CLD2::uint32 bytesPerBucket = sizeof(CLD2::IndirectProbBucket4);
    CLD2::uint32 numBuckets = summary->kCLDTableSize;
    CLD2::uint32 tableSizeBytes = bytesPerBucket * numBuckets;
    CLD2::uint32 indirectTableSizeBytes =
      supplement->indirectTableSizes[x] * sizeof(CLD2::uint32);
    CLD2::uint32 recognizedScriptsSizeBytes =
      strlen(summary->kRecognizedLangScripts) + 1; // note null terminator

    // Place size information into header. We'll align on byte boundaries later.
    tableHeader.lengthOf_kCLDTable = tableSizeBytes;
    tableHeader.lengthOf_kCLDTableInd = indirectTableSizeBytes;
    tableHeader.lengthOf_kRecognizedLangScripts =
      recognizedScriptsSizeBytes; // null terminator counted above
  }
}

// Assuming that all fields have been set in the specified header, re-align
// the starting positions of all data chunks to be aligned along 64-bit
// boundaries for maximum efficiency.
void alignAll(CLD2DynamicData::FileHeader* header, const int alignment) {
  CLD2::uint32 totalPadding = 0;
  if (DEBUG) { fprintf(stdout, "Align for %d bits.\n", (alignment*8)); }
  CLD2::uint32 headerSize = CLD2DynamicData::calculateHeaderSize(
    header->numTablesEncoded);
  CLD2::uint32 offset = headerSize;

  { // scoping block
    int stateTablePad = alignment - (offset % alignment);
    if (stateTablePad == alignment) stateTablePad = 0;
    totalPadding += stateTablePad;
    if (DEBUG) { fprintf(stdout, "Alignment for stateTable adjusted by %d\n", stateTablePad); }
    offset += stateTablePad;
    header->startOf_utf8PropObj_state_table = offset;
    offset += header->lengthOf_utf8PropObj_state_table;
  }

  { // scoping block
    int remapPad = alignment - (offset % alignment);
    if (remapPad == alignment) remapPad = 0;
    totalPadding += remapPad;
    if (DEBUG) { fprintf(stdout, "Alignment for remap adjusted by %d\n", remapPad); }
    offset += remapPad;
    header->startOf_utf8PropObj_remap_base = offset;
    offset += header->lengthOf_utf8PropObj_remap_base;
  }

  { // scoping block
    int remapStringPad = alignment - (offset % alignment);
    if (remapStringPad == alignment) remapStringPad = 0;
    totalPadding += remapStringPad;
    if (DEBUG) { fprintf(stdout, "Alignment for remapString adjusted by %d\n", remapStringPad); }
    offset += remapStringPad;
    header->startOf_utf8PropObj_remap_string = offset;
    offset += header->lengthOf_utf8PropObj_remap_string; // null terminator already counted in initUtf8Headers
  }

  { // scoping block
    int fastStatePad = alignment - (offset % alignment);
    if (fastStatePad == alignment) fastStatePad = 0;
    totalPadding += fastStatePad;
    if (DEBUG) { fprintf(stdout, "Alignment for fastState adjusted by %d\n", fastStatePad); }
    offset += fastStatePad;
    if (header->lengthOf_utf8PropObj_fast_state > 0) {
      header->startOf_utf8PropObj_fast_state = offset;
      offset += header->lengthOf_utf8PropObj_fast_state; // null terminator already counted in initUtf8Headers
    } else {
      header->startOf_utf8PropObj_fast_state = 0;
    }
  }

  { // scoping block
    int deltaOctaPad = alignment - (offset % alignment);
    if (deltaOctaPad == alignment) deltaOctaPad = 0;
    totalPadding += deltaOctaPad;
    if (DEBUG) { fprintf(stdout, "Alignment for deltaOctaScore adjusted by %d\n", deltaOctaPad); }
    offset += deltaOctaPad;
    header->startOf_kAvgDeltaOctaScore = offset;
    offset += header->lengthOf_kAvgDeltaOctaScore;
  }
  
  for (int x=0; x<header->numTablesEncoded; x++) {
    CLD2DynamicData::TableHeader& tableHeader = header->tableHeaders[x];
    int tablePad = alignment - (offset % alignment);
    if (tablePad == alignment) tablePad = 0;
    totalPadding += tablePad;
    if (DEBUG) { fprintf(stdout, "Alignment for table %d adjusted by %d\n", x, tablePad); }
    offset += tablePad;
    tableHeader.startOf_kCLDTable = offset;
    offset += tableHeader.lengthOf_kCLDTable;

    int indirectPad = alignment - (offset % alignment);
    if (indirectPad == alignment) indirectPad = 0;
    totalPadding += indirectPad;
    if (DEBUG) { fprintf(stdout, "Alignment for tableInd %d adjusted by %d\n", x, indirectPad); }
    offset += indirectPad;
    tableHeader.startOf_kCLDTableInd = offset;
    offset += tableHeader.lengthOf_kCLDTableInd;

    int scriptsPad = alignment - (offset % alignment);
    if (scriptsPad == alignment) scriptsPad = 0;
    totalPadding += scriptsPad;
    if (DEBUG) { fprintf(stdout, "Alignment for scriptsPad %d adjusted by %d", x, scriptsPad); }
    offset += scriptsPad;
    tableHeader.startOf_kRecognizedLangScripts = offset;
    offset += tableHeader.lengthOf_kRecognizedLangScripts; // null terminator already counted in initTableHeaders
  }

  // Now that we know exactly how much data we have written, store it in the
  // header as a sanity check
  header->totalFileSizeBytes = offset;

  if (DEBUG) {
    fprintf(stdout, "Data aligned.\n");
    fprintf(stdout, "Header size:  %d bytes\n", headerSize);
    fprintf(stdout, "Data size:    %d bytes\n", (offset - totalPadding));
    fprintf(stdout, "Padding size: %d bytes\n", totalPadding);
    fprintf(stdout, "  cld_generated_CjkUni_obj: %d bytes\n", (
        header->lengthOf_utf8PropObj_state_table +
        header->lengthOf_utf8PropObj_remap_string +
        header->lengthOf_utf8PropObj_fast_state));
    fprintf(stdout, "  kAvgDeltaOctaScore:       %d bytes\n",
        header->lengthOf_kAvgDeltaOctaScore);
    fprintf(stdout, "  kCjkCompat_obj:           %d bytes\n", (
        header->tableHeaders[0].lengthOf_kCLDTable +
        header->tableHeaders[0].lengthOf_kCLDTableInd +
        header->tableHeaders[0].lengthOf_kRecognizedLangScripts + 1));
    fprintf(stdout, "  kCjkDeltaBi_obj:          %d bytes\n", (
        header->tableHeaders[1].lengthOf_kCLDTable +
        header->tableHeaders[1].lengthOf_kCLDTableInd +
        header->tableHeaders[1].lengthOf_kRecognizedLangScripts + 1));
    fprintf(stdout, "  kDistinctBiTable_obj:     %d bytes\n", (
        header->tableHeaders[2].lengthOf_kCLDTable +
        header->tableHeaders[2].lengthOf_kCLDTableInd +
        header->tableHeaders[2].lengthOf_kRecognizedLangScripts + 1));
    fprintf(stdout, "  kQuad_obj:                %d bytes\n", (
        header->tableHeaders[3].lengthOf_kCLDTable +
        header->tableHeaders[3].lengthOf_kCLDTableInd +
        header->tableHeaders[3].lengthOf_kRecognizedLangScripts + 1));
    fprintf(stdout, "  kQuad_obj2:               %d bytes\n", (
        header->tableHeaders[4].lengthOf_kCLDTable +
        header->tableHeaders[4].lengthOf_kCLDTableInd +
        header->tableHeaders[4].lengthOf_kRecognizedLangScripts + 1));
    fprintf(stdout, "  kDeltaOcta_obj:           %d bytes\n", (
        header->tableHeaders[5].lengthOf_kCLDTable +
        header->tableHeaders[5].lengthOf_kCLDTableInd +
        header->tableHeaders[5].lengthOf_kRecognizedLangScripts + 1));
    fprintf(stdout, "  kDistinctOcta_obj:        %d bytes\n", (
        header->tableHeaders[6].lengthOf_kCLDTable +
        header->tableHeaders[6].lengthOf_kCLDTableInd +
        header->tableHeaders[6].lengthOf_kRecognizedLangScripts + 1));
  }
}

void initDeltaHeaders(CLD2DynamicData::FileHeader* header, const CLD2::uint32 deltaLength) {
  header->startOf_kAvgDeltaOctaScore = 0;
  header->lengthOf_kAvgDeltaOctaScore = deltaLength;
}

void initUtf8Headers(CLD2DynamicData::FileHeader* header, const CLD2::UTF8PropObj* utf8Object) {
  header->utf8PropObj_state0 = utf8Object->state0;
  header->utf8PropObj_state0_size = utf8Object->state0_size;
  header->utf8PropObj_total_size = utf8Object->total_size;
  header->utf8PropObj_max_expand = utf8Object->max_expand;
  header->utf8PropObj_entry_shift = utf8Object->entry_shift;
  header->utf8PropObj_bytes_per_entry = utf8Object->bytes_per_entry;
  header->utf8PropObj_losub = utf8Object->losub;
  header->utf8PropObj_hiadd = utf8Object->hiadd;
  header->lengthOf_utf8PropObj_state_table = utf8Object->total_size;
  header->lengthOf_utf8PropObj_remap_base = sizeof(CLD2::RemapEntry); // TODO: Can this ever have more than one entry?
  header->lengthOf_utf8PropObj_remap_string = strlen(
    reinterpret_cast<const char*>(utf8Object->remap_string)) + 1; // note null terminator
  if (utf8Object->fast_state == NULL) {
    header->lengthOf_utf8PropObj_fast_state = 0; // not applicable
  } else {
    header->lengthOf_utf8PropObj_fast_state = strlen(
      reinterpret_cast<const char*>(utf8Object->fast_state)) + 1; // note null terminator
  }
}
} // End namespace CLD2DynamicDataExtractor
