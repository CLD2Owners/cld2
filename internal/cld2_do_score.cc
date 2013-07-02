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

// Little program to read lines of sample text, calculate score per 1024 bytes
// per language-script4 combination
// Possible input file /export/hda3/cld/pre2010/b0_samp_prune_20100722.utf8

#include <stdio.h>
#include <string.h>
#include <string>

#include "compact_lang_det_impl.h"
#include "lang_script.h"

using namespace std;
using namespace CLD2;

double bytes[NUM_LANGUAGES][4];
double scores[NUM_LANGUAGES][4];


// Return score per 1024 bytes for top language
Language ScoreOneLine(const char* buffer, int buffer_length,
                    int* bytes, double* score_per_1kb) {
  bool is_plain_text = true;
  const CLDHints* cld_hints = NULL;
  bool allow_extended_lang = true;
  int flags = 0;
  Language plus_one = UNKNOWN_LANGUAGE;
  Language language3[3];
  int percent3[3];
  double normalized_score3[3];
  ResultChunkVector* resultchunkvector = NULL;
  int text_bytes;
  bool is_reliable;
  Language summary_lang;

  summary_lang = DetectLanguageSummaryV2(
                        buffer,
                        buffer_length,
                        is_plain_text,
                        cld_hints,
                        allow_extended_lang,
                        flags,
                        plus_one,
                        language3,
                        percent3,
                        normalized_score3,
                        resultchunkvector,
                        &text_bytes,
                        &is_reliable);
  *bytes = text_bytes;
  *score_per_1kb = normalized_score3[0];
  return language3[0];
}

#define LF 0x0a
#define CR 0x0d
const int kMaxBuffer = 5 * 1024;

bool ReadLine(FILE* infile, char* buffer, size_t maxlen) {
  char* p = fgets(buffer, maxlen, infile);
  if (p == NULL) {
    return false;
  }
  int len = strlen(buffer);

  // trim CR LF
  if (buffer[len-1] == LF) {buffer[--len] = '\0';}
  if (buffer[len-1] == CR) {buffer[--len] = '\0';}
  return true;
}

bool IsComment(const char* buffer) {
  int len = strlen(buffer);
  if (len == 0) {return true;}
  if (buffer[0] == '#') {return true;}
  if (buffer[0] == ' ') {return true;}    // Any leading space is comment
  return false;
}

// Skips over xxxxx_ where _ is one or more spaces/tabs
// Returns string::npos if no more fields
int SkipOneField(const string& src, int pos) {
  if (pos == string::npos) {return pos;}

  int lpos = pos;
  lpos = src.find_first_of(" \t", lpos);
  if (lpos == string::npos) {return lpos;}
  lpos = src.find_first_not_of(" \t", lpos);
  if (lpos == string::npos) {return lpos;}
  return lpos;
}

// Return language and script from parsed line or defaults
void GetLangScript(const string& src,
                   Language default_lang, ULScript default_lscript,
                   Language* target_lang, ULScript* target_lscript,
                   string* tld) {
  *target_lang = default_lang;
  *target_lscript = default_lscript;
  *tld = "";
  int pos = 0;
  int pos2 = 0;
  if (src.substr(0,7) == "SAMPLE ") {
    // SAMPLE ll-Ssss
    pos = SkipOneField(src, pos);
  } else if (src.substr(0,5) == "SAMP ") {
    // SAMP ll-Ssss /tld2.tld/
    pos = SkipOneField(src, pos);
    pos2 = SkipOneField(src, pos);
  } else if (src.substr(0,5) == "Samp ") {
    // Samp ll-Ssss /tld2.tld/
    pos = SkipOneField(src, pos);
    pos2 = SkipOneField(src, pos);
  }
  if (pos == 0) {return;}
  if (pos == string::npos) {return;}

  // Pos is at the first letter of language-script combination
  int end = src.find_first_of(" \t", pos);    // find end of lang-script
  if (end == string::npos) {return;}
  *target_lang = GetLanguageFromName(src.substr(pos, end - pos).c_str());
  *target_lscript = GetULScriptFromName(src.substr(pos, end - pos).c_str());

  // Pos2 is 0 or at the first letter of the tld string
  if (pos2 == 0) {return;}
  if (pos2 == string::npos) {return;}
  end = src.find_first_of(" \t", pos2);
  if (end == string::npos) {return;}
  *tld = src.substr(pos2, end - pos2);
}

// Return position of start of text
int GetTextBeginPos(const string& src) {
  int pos = 0;
  if (src.size() < 8) {return pos;}

  if (src.substr(0,7) == "SAMPLE ") {
    // Skip SAMPLE ll-Ssss
    pos = SkipOneField(src, pos);
    pos = SkipOneField(src, pos);
  } else if (src.substr(0,5) == "SAMP ") {
    // Skip SAMP ll-Ssss /tld2.tld/
    pos = SkipOneField(src, pos);
    pos = SkipOneField(src, pos);
    pos = SkipOneField(src, pos);
  } else if (src.substr(0,5) == "Samp ") {
    // Skip Samp ll-Ssss /tld2.tld/
    pos = SkipOneField(src, pos);
    pos = SkipOneField(src, pos);
    pos = SkipOneField(src, pos);
  }
  return pos;
}

// Avoid zdiv
inline double Divisor(double x) {
   return (x > 0.0 ? x : 1.0);
}

void Flush(Language cur_lang, ULScript ulscript,
           double total_score_cur_lang,
           double total_bytes_cur_lang, double total_bad_bytes_cur_lang) {
  if (cur_lang == UNKNOWN_LANGUAGE) {return;}

  bytes[cur_lang][LScript4(ulscript)] += total_bytes_cur_lang;
  scores[cur_lang][LScript4(ulscript)] += total_score_cur_lang;

  double score = total_score_cur_lang * 1024.0 / Divisor(total_bytes_cur_lang);
  double percent_bad = 100.0 * total_bad_bytes_cur_lang /
     Divisor(total_bytes_cur_lang + total_bad_bytes_cur_lang);
  fprintf(stdout, "%s-%s    %7.0f %6.1f, %2.0f%% bad SUMMARY\n\n",
          LanguageCode(cur_lang),
          ULScriptCode(ulscript),
          total_bytes_cur_lang,
          score,
          percent_bad);
}

int BytesPer1KB(int i, int j) {
   int bytes_per_1kb = ((scores[i][j] * 1024.0) / Divisor(bytes[i][j])) + 0.5;
   return bytes_per_1kb;
}

int main(int argc, char *argv[]) {
  Language cur_lang = UNKNOWN_LANGUAGE;
  ULScript cur_ulscript = ULScript_Common;
  double total_score_cur_lang = 0.0;
  double total_bytes_cur_lang = 0.0;
  double total_bad_bytes_cur_lang = 0.0;
  memset(bytes, 0, sizeof(bytes));
  memset(scores, 0, sizeof(bytes));

  char buffer[kMaxBuffer];
  int buffer_length;
  const char* filename = NULL;
  FILE* infile = stdin;
  for (int i = 1; i < argc; ++i) {
     if (argv[i][0] != '-') {
        filename = argv[i];
     }
  }

  if (filename != NULL) {
     infile = fopen(filename, "r");
     if (infile == NULL) {
        fprintf(stderr, "%s did not open\n", filename);
        return 0;
     }
  }

  while (ReadLine(infile, buffer, kMaxBuffer)) {
    if (IsComment(buffer)) {continue;}

    buffer_length = strlen(buffer);
    int bytes;
    double score_per_1kb;
    Language toplang;
    Language target_lang;
    ULScript target_ulscript;

    string src(buffer, buffer_length);
    string tld("");
    int pos = GetTextBeginPos(src);
    GetLangScript(src, UNKNOWN_LANGUAGE, ULScript_Common,
                   &target_lang, &target_ulscript, &tld);
    if ((cur_lang != target_lang) || (cur_ulscript != target_ulscript)) {
      Flush(cur_lang, cur_ulscript, total_score_cur_lang,
            total_bytes_cur_lang, total_bad_bytes_cur_lang);
      cur_lang = target_lang;
      cur_ulscript = target_ulscript;
      total_score_cur_lang = 0.0;
      total_bytes_cur_lang = 0.0;
      total_bad_bytes_cur_lang = 0.0;
    }

    toplang = ScoreOneLine(&src[pos], src.size() - pos, &bytes, &score_per_1kb);

    fprintf(stdout, "%s%c %d %4.1f %s\n",
            LanguageCode(toplang),
            (toplang == target_lang) ? ' ' : '*',
            bytes, score_per_1kb, buffer);
    // Only count when detected lang matches the claimed target lang
    if (toplang == target_lang) {
      total_bytes_cur_lang += bytes;
      total_score_cur_lang += (score_per_1kb * bytes) / 1024.0;
    } else {
      total_bad_bytes_cur_lang += bytes;
    }
  }
  Flush(cur_lang, cur_ulscript, total_score_cur_lang,
        total_bytes_cur_lang, total_bad_bytes_cur_lang);

  for (int i = 0; i < NUM_LANGUAGES; ++i) {
     Language ilang = static_cast<Language>(i);
     fprintf(stdout, "  {%4d, %4d, %4d, %4d},  // %d %s %s\n",
             BytesPer1KB(i, 0), BytesPer1KB(i, 1),
             BytesPer1KB(i, 2), BytesPer1KB(i, 3),
             i, LanguageName(ilang), LanguageCode(ilang));
  }

  if (infile != stdin) {
     fclose(infile);
  }
}
