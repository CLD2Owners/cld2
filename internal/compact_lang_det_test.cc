// Copyright 2013 Google Inc. All Rights Reserved.
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

//
// Author: dsites@google.com (Dick Sites)
//

// Test: Do encoding detection on input file
//       --line treat each line as a separate detection problem

#include <math.h>                   // for sqrt
#include <stdlib.h>                 // for exit
#include <stdio.h>
#include <string.h>
#include <sys/time.h>               // for gettimeofday
#include <string>

#include "cld2tablesummary.h"
#include "compact_lang_det_impl.h"
#include "debug.h"
#include "integral_types.h"
#include "lang_script.h"
#include "utf8statetable.h"

namespace CLD2 {

using namespace std;

// Scaffolding
typedef int32 Encoding;
static const Encoding UNKNOWN_ENCODING = 0;


#ifndef CLD2_DYNAMIC_MODE
// Linker supplies the right tables; see ScoringTables compact_lang_det_impl.cc
// These are here JUST for printing versions
extern const UTF8PropObj cld_generated_CjkUni_obj;
extern const CLD2TableSummary kCjkDeltaBi_obj;
extern const CLD2TableSummary kDistinctBiTable_obj;
extern const CLD2TableSummary kQuad_obj;
extern const CLD2TableSummary kDeltaOcta_obj;
extern const CLD2TableSummary kDistinctOcta_obj;
extern const CLD2TableSummary kOcta2_obj;
extern const short kAvgDeltaOctaScore[];
#endif

bool FLAGS_cld_version = false;
bool FLAGS_cld_html = true;
int32 FLAGS_repeat = 1;
bool FLAGS_plain = false;
bool FLAGS_dbgscore = true;


// Convert GetTimeOfDay output to 64-bit usec
static inline uint64 Microseconds(const struct timeval& t) {
  // Convert to (uint64) microseconds,  not (double) seconds.
  return t.tv_sec * 1000000ULL + t.tv_usec;
}

#define LF 0x0a
#define CR 0x0d

bool Readline(FILE* infile, char* buffer) {
  char* p = fgets(buffer, 64 * 1024, infile);
  if (p == NULL) {
    return false;
  }
  int len = strlen(buffer);

  // trim CR LF
  if (buffer[len-1] == LF) {buffer[--len] = '\0';}
  if (buffer[len-1] == CR) {buffer[--len] = '\0';}
  return true;
}

bool IsComment(char* buffer) {
  int len = strlen(buffer);
  if (len == 0) {return true;}
  if (buffer[0] == '#') {return true;}
  if (buffer[0] == ' ') {return true;}    // Any leading space is comment
  return false;
}



void DumpExtLang(int flags,
                 Language summary_lang,
                 Language* language3, int* percent3,
                 double* normalized_score3,
                 int text_bytes, bool is_reliable, int in_size) {
  char temp[160];
  char* tp = temp;
  int tp_left = sizeof(temp);
  snprintf(tp, tp_left, "ExtLanguage");

  if (language3[0] != UNKNOWN_LANGUAGE) {
    tp = temp + strlen(temp);
    tp_left = sizeof(temp) - strlen(temp);
    snprintf(tp, tp_left, " %s(%d%% %3.0fp)",
             LanguageName(language3[0]),
             percent3[0],
             normalized_score3[0]);

  }
  if (language3[1] != UNKNOWN_LANGUAGE) {
    tp = temp + strlen(temp);
    tp_left = sizeof(temp) - strlen(temp);
    snprintf(tp, tp_left, ", %s(%d%% %3.0fp)",
             LanguageName(language3[1]),
             percent3[1],
             normalized_score3[1]);
  }
  if (language3[2] != UNKNOWN_LANGUAGE) {
    tp = temp + strlen(temp);
    tp_left = sizeof(temp) - strlen(temp);
    snprintf(tp, tp_left, ", %s(%d%% %3.0fp)",
             LanguageName(language3[2]),
             percent3[2],
             normalized_score3[2]);
  }

  if (text_bytes > 9999) {
    tp = temp + strlen(temp);
    tp_left = sizeof(temp) - strlen(temp);
    snprintf(tp, tp_left, ", %d/%d KB of non-tag letters",
            text_bytes >> 10, in_size >> 10);
  } else {
    tp = temp + strlen(temp);
    tp_left = sizeof(temp) - strlen(temp);
    snprintf(tp, tp_left, ", %d/%d bytes of non-tag letters",
            text_bytes, in_size);
  }

  tp = temp + strlen(temp);
  tp_left = sizeof(temp) - strlen(temp);
  snprintf(tp, tp_left, ", Summary: %s%s",
           LanguageName(summary_lang),
           is_reliable ? "" : "*");

  printf("%s\n", temp);

  // Also put into optional HTML output
  if ((flags & kCLDFlagHtml) != 0) {
    fprintf(stderr, "%s\n", temp);
  }
}

void DumpLanguages(Language summary_lang,
                   Language* language3, int* percent3,
                 int text_bytes, bool is_reliable, int in_size) {
  // fprintf(stderr, "</span>\n\n");
  int total_percent = 0;
  if (language3[0] != UNKNOWN_LANGUAGE) {
    fprintf(stderr, "\n<br>Languages %s(%d%%)",
            LanguageName(language3[0]),
            percent3[0]);
    total_percent += percent3[0];
  } else {
    fprintf(stderr, "\n<br>Languages ");
  }

  if (language3[1] != UNKNOWN_LANGUAGE) {
    fprintf(stderr, ", %s(%d%%)",
            LanguageName(language3[1]),
            percent3[1]);
    total_percent += percent3[1];
  }

  if (language3[2] != UNKNOWN_LANGUAGE) {
    fprintf(stderr, ", %s(%d%%)",
            LanguageName(language3[2]),
            percent3[2]);
    total_percent += percent3[2];
  }

  fprintf(stderr, ", other(%d%%)", 100 - total_percent);

  if (text_bytes > 9999) {
    fprintf(stderr, ", %d/%d KB of non-tag letters",
            text_bytes >> 10, in_size >> 10);
  } else {
    fprintf(stderr, ", %d/%d bytes of non-tag letters",
            text_bytes, in_size);
  }

  fprintf(stderr, ", Summary: %s%s ",
          LanguageName(summary_lang),
          is_reliable ? "" : "*");
  fprintf(stderr, "<br>\n");
}


int main(int argc, char** argv) {
  if (FLAGS_cld_version) {
#ifndef CLD2_DYNAMIC_MODE
    printf("%s %4dKB uni build date, bytes\n",
           "........",
           cld_generated_CjkUni_obj.total_size >> 10);
    printf("%d %4ldKB delta_bi build date, bytes\n",
           kCjkDeltaBi_obj.kCLDTableBuildDate,
           (kCjkDeltaBi_obj.kCLDTableSize *
            sizeof(IndirectProbBucket4)) >> 10);
    printf("%d %4ldKB quad build date, bytes\n",
           kQuad_obj.kCLDTableBuildDate,
           (kQuad_obj.kCLDTableSize *
            sizeof(IndirectProbBucket4)) >> 10);
    printf("%d %4ldKB delta_octa build date, bytes\n",
           kDeltaOcta_obj.kCLDTableBuildDate,
           (kDeltaOcta_obj.kCLDTableSize *
            sizeof(IndirectProbBucket4)) >> 10);
#else
    printf("FLAGS_cld_version doesn't work with dynamic data mode\n");
#endif
    exit(0);
  }     // End FLAGS_cld_version

  int flags = 0;
  bool get_vector = false;
  const char* data_file = NULL;
  bool do_line = false;
  const char* fname = NULL;
  for (int i = 1; i < argc; ++i) {
    if (argv[i][0] != '-') {fname = argv[i];}
    if (strcmp(argv[i], "--scoreasquads") == 0) {flags |= kCLDFlagScoreAsQuads;}
    if (strcmp(argv[i], "--html") == 0) {flags |= kCLDFlagHtml;}
    if (strcmp(argv[i], "--cr") == 0) {flags |= kCLDFlagCr;}
    if (strcmp(argv[i], "--verbose") == 0) {flags |= kCLDFlagVerbose;}
    if (strcmp(argv[i], "--echo") == 0) {flags |= kCLDFlagEcho;}
    if (strcmp(argv[i], "--besteffort") == 0) {flags |= kCLDFlagBestEffort;}
    if (strcmp(argv[i], "--vector") == 0) {get_vector = true;}
    if (strcmp(argv[i], "--line") == 0) {do_line = true;}
    if (strcmp(argv[i], "--data-file") == 0) { data_file = argv[++i];}
  }

#ifdef CLD2_DYNAMIC_MODE        
  if (data_file == NULL) {      
    fprintf(stderr, "When running in dynamic mode, you must specify --data-file [FILE]\n");     
    return -1;      
  }     
  fprintf(stdout, "Loading data from: %s\n", data_file);        
  CLD2::loadDataFromFile(data_file);        
  fprintf(stdout, "Data loaded, test commencing\n");        
#endif

  FILE* fin;
  if (fname == NULL) {
    fin = stdin;
  } else {
    if (do_line) {
      fin = fopen(fname, "r");
    } else {
      fin = fopen(fname, "rb");
    }
    if (fin == NULL) {
      fprintf(stderr, "%s did not open\n", fname);
      exit(0);
    }
  }

  const char* tldhint = "";
  Encoding enchint = UNKNOWN_ENCODING;
  Language langhint = UNKNOWN_LANGUAGE;

  int bytes_consumed;
  int bytes_filled;
  int error_char_count;
  bool is_reliable;
  int usec;
  char* buffer = new char[10000000];  // Max 10MB of input for this test program
  struct timeval news, newe;

  // Full-blown flag-bit and hints interface
  bool allow_extended_lang = true;
  Language plus_one = UNKNOWN_LANGUAGE;
  bool ignore_7bit = false;

  if (do_line) {
    while (Readline(fin, buffer)) {
      if (IsComment(buffer)) {continue;}

      // Detect language one line at a time
      Language summary_lang = UNKNOWN_LANGUAGE;

      Language language3[3];
      int percent3[3];
      double normalized_score3[3];
      ResultChunkVector resultchunkvector;
      bool is_plain_text = FLAGS_plain;
      int text_bytes;

      CLDHints cldhints = {NULL, tldhint, enchint, langhint};

      summary_lang = CLD2::DetectLanguageSummaryV2(
                          buffer,
                          strlen(buffer),
                          is_plain_text,
                          &cldhints,
                          allow_extended_lang,
                          flags,
                          plus_one,
                          language3,
                          percent3,
                          normalized_score3,
                          get_vector ? &resultchunkvector : NULL,
                          &text_bytes,
                          &is_reliable);
      printf("%s%s %d%% %s\n",
         LanguageName(language3[0]),
         is_reliable ? "" : "*",
         percent3[0],
         buffer);
    }
    fclose(fin);
    delete[] buffer;
    return 0;
  }

  if ((flags & kCLDFlagHtml) != 0) {
    // Begin HTML file
    fprintf(stderr, "<html><meta charset=\"UTF-8\"><body>\n");
    fprintf(stderr, "<style media=\"print\" type=\"text/css\"> "
                    ":root { -webkit-print-color-adjust: exact; } </style>\n");
    fprintf(stderr, "<span style=\"font-size: 7pt\">\n");
  }

  if ((flags & kCLDFlagHtml) != 0) {
    //// fprintf(stderr, "<html><body><span style=\"font-size: 7pt\">\n");
    //// fprintf(stderr, "<html><body><span style=\"font-size: 6pt\"><pre>\n");
    fprintf(stderr, "file = %s<br>\n", fname ? fname : "stdin");
  }

  // Read entire file
  int n = fread(buffer, 1, 10000000, fin);


  // Detect languages in entire file
  Language summary_lang = UNKNOWN_LANGUAGE;

  Language language3[3];
  int percent3[3];
  double normalized_score3[3];
  ResultChunkVector resultchunkvector;
  bool is_plain_text = FLAGS_plain;
  int text_bytes;

  CLDHints cldhints = {NULL, tldhint, enchint, langhint};

  gettimeofday(&news, NULL);
  for (int i = 0; i < FLAGS_repeat; ++i) {
    summary_lang = CLD2::DetectLanguageSummaryV2(
                          buffer,
                          n,
                          is_plain_text,
                          &cldhints,
                          allow_extended_lang,
                          flags,
                          plus_one,
                          language3,
                          percent3,
                          normalized_score3,
                          get_vector ? &resultchunkvector : NULL,
                          &text_bytes,
                          &is_reliable);
  }
  gettimeofday(&newe, NULL);

  if (get_vector) {
    DumpResultChunkVector(stderr, buffer, &resultchunkvector);
  }

  DumpExtLang(flags, summary_lang, language3, percent3, normalized_score3,
              text_bytes, is_reliable, n);

  if ((flags & kCLDFlagHtml) != 0) {
    DumpLanguages(summary_lang,
                  language3, percent3, text_bytes, is_reliable, n);
  }

  usec = static_cast<int>(Microseconds(newe) - Microseconds(news));
  if (usec == 0) {usec = 1;}
  printf("  SummaryLanguage %s%s at %u of %d %uus (%d MB/sec), %s\n",
         LanguageName(summary_lang),
         is_reliable ? "" : "(un-reliable)",
         bytes_consumed,
         n,
         usec,
         n / usec,
         argv[1]);

  if ((flags & kCLDFlagHtml) != 0) {
    fprintf(stderr, "\n</span></body></html><br>");
  }

  fclose(fin);
  delete[] buffer;

  return 0;
}

}       // End namespace CLD2

int main(int argc, char *argv[]) {
  return CLD2::main(argc, argv);
}

