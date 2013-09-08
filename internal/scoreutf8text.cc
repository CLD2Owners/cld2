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
// Little program to read sample UTF-8 text and score it
// Giving precision, recall, F, and matrix
//
// Author: dsites@google.com (Dick Sites)
//

#include <math.h>                   // for sqrt
#include <stdio.h>
#include <string.h>
#include <string>

#include "debug.h"        // for uint8 etc
#include "integral_types.h"        // for uint8 etc
#include "compact_lang_det_impl.h"
#include "lang_script.h"

using namespace std;

namespace CLD2 {


// Scaffolding
typedef int32 Encoding;
static const Encoding UNKNOWN_ENCODING = 0;

static const bool FLAGS_cld2_html = true;
static const bool FLAGS_noext = false;
static const bool FLAGS_echo_mismatch = true;
static const int32 FLAGS_minsize = 0;


/***
accepts one or more input files
loop:
 reads source line
 does cld on each source line
 records source lang-script, CLD lang-script, count+=1

at end, print row headers = CLD lang, script, per_M
print column headers = in lang, script, per_M
print matrix, recall, precision, F
and overall RMS F.

sort by script, and within script, by per_M and near diagonal
***/


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

bool IsComment(char* buffer) {
  int len = strlen(buffer);
  if (len == 0) {return true;}
  if (buffer[0] == '#') {return true;}
  if (buffer[0] == ' ') {return true;}    // Any leading space is comment
  if ((len >= 5) && (memcmp(buffer, "BOGUS", 5) == 0)) {return true;}
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

// Return language and script from parsed line
void GetStatedLangScript(const string& src, string* lang_script, string* tld) {
  *lang_script = "";
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
  *lang_script = src.substr(pos, end - pos);

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


bool CarefulMatch(const char* in_langscript,
                  Language in_lang, ULScript in_lscript,
                  Language cld_lang, ULScript cld_lscript) {
  bool easy_match = ((in_lang == cld_lang) & (in_lscript == cld_lscript));
  if (easy_match) {return true;}

  // Unrecognized list, matching un-Xxxx
  if ((cld_lang == UNKNOWN_LANGUAGE) && (in_lscript == cld_lscript)) {
    if (strcmp(in_langscript, "az-Arab") == 0) {return true;}
    if (strcmp(in_langscript, "az-Cyrl") == 0) {return true;}
    if (strcmp(in_langscript, "kk-Latn") == 0) {return true;}
    if (strcmp(in_langscript, "ku-Latn") == 0) {return true;}
    if (strcmp(in_langscript, "my-Latn") == 0) {return true;}
    if (strcmp(in_langscript, "ru-Latn") == 0) {return true;}
    if (strcmp(in_langscript, "tg-Arab") == 0) {return true;}
    if (strcmp(in_langscript, "ug-Latn") == 0) {return true;}
    if (strcmp(in_langscript, "za-Hani") == 0) {return true;}
  }

  // bs/me => sr/hr
  if ((cld_lang == CROATIAN) && (cld_lscript == ULScript_Latin)) {
    if (strcmp(in_langscript, "bs-Latn") == 0) {return true;}
    if (strcmp(in_langscript, "sr-ME-Latn") == 0) {return true;}
  }
  if ((cld_lang == SERBIAN) && (cld_lscript == ULScript_Cyrillic)) {
    if (strcmp(in_langscript, "bs-Cyrl") == 0) {return true;}
    if (strcmp(in_langscript, "sr-ME-Cyrl") == 0) {return true;}
  }

  // Twi => Akan
  if ((cld_lang == AKAN) && (cld_lscript == ULScript_Latin)) {
    if (strcmp(in_langscript, "tw-Latn") == 0) {return true;}
  }

  // za-Hani
  if ((cld_lang == CHINESE) && (cld_lscript == ULScript_Hani)) {
    if (strcmp(in_langscript, "za-Hani") == 0) {return true;}
  }

  // zzb, zze, zzh fake languages
  if (strcmp(in_langscript, "zzb-Latn") == 0) {return true;}
  if (strcmp(in_langscript, "zze-Latn") == 0) {return true;}
  if (strcmp(in_langscript, "zzh-Latn") == 0) {return true;}


  return false;
}


#if 0
typedef hash_map<string, int32> StringIntMap;

static int32 next_in_map;
static int32 next_cld_map;
static int32 next_in_cld_map;
static StringIntMap in_map;         // xx-Fooo to small int
static StringIntMap cld_map;        // xx-Fooo to small int
static StringIntMap in_cld_map;     // xx-Fooo_xx-Barr to small int

static vector<int32> in_count;      // counts by in_map subscript
static vector<int32> cld_count;     // counts by cld_map subscript
static vector<int32> in_cld_count;  // counts by in_cld_map subscript

int32 MapToSmallInt(const string& s, StringIntMap* smap, int* next_smap) {
  StringIntMap::iterator it = smap->find(s);
  if (it == smap->end()) {
    // New
    (*smap)[s] = *next_smap;
    *next_smap += 1;
  }
  return (*smap)[s];
}
#endif


void InitResult() {
#if 0
  in_map.clear();
  cld_map.clear();
  in_cld_map.clear();
  next_in_map = 0;
  next_cld_map = 0;
  next_in_cld_map = 0;
  in_count.clear();
  cld_count.clear();
  in_cld_count.clear();
#endif
}

void RecordCLDResult(const char* buffer, const char* in_langscript,
                     Language in_lang, ULScript in_lscript,
                     Language cld_lang, ULScript cld_lscript) {

  bool match = CarefulMatch(in_langscript,
                            in_lang, in_lscript, cld_lang, cld_lscript);
  if (FLAGS_echo_mismatch && !match) {
    fprintf(stderr,
            "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;=Mismatch: "
            "expected %s, actual %s<br>\n",
            LanguageCode(in_lang), LanguageCode(cld_lang));
  }
#if 0
  printf("%s %s-%s %s\n", in_langscript,
         ExtLanguageCode(cld_lang), UnicodeLScriptCode(cld_lscript),
         match ? "" : "!=");

  string cld_langscript = ExtLanguageCode(cld_lang);
  cld_langscript.append("-");
  cld_langscript.append( UnicodeLScriptCode(cld_lscript));

  string in_cld_langscript = in_langscript;
  in_cld_langscript.append("_");
  in_cld_langscript.append(cld_langscript);

  // Extend vectors if needed
  int32 in_int = MapToSmallInt(in_langscript, &in_map, &next_in_map);
  while (in_count.size() <= in_int) {in_count.push_back(0);}
  in_count[in_int] += 1;

  int32 cld_int = MapToSmallInt(cld_langscript, &cld_map, &next_cld_map);
  while (cld_count.size() <= cld_int) {cld_count.push_back(0);}
  cld_count[cld_int] += 1;

  int32 in_cld_int = MapToSmallInt(in_cld_langscript,
                                   &in_cld_map, &next_in_cld_map);
  while (in_cld_count.size() <= in_cld_int) {in_cld_count.push_back(0);}
  in_cld_count[in_cld_int] += 1;
#endif
}

void FinishResult() {
#if 0
  int32 in_n = in_map.size();
  int32 cld_n = cld_map.size();

  int32* in_total = new int32[in_n];
  memset(in_total, 0, in_n * sizeof(int32));
  int32* in_matches = new int32[in_n];
  memset(in_matches, 0, in_n * sizeof(int32));
  string* in_str = new string[in_n];
  for (StringIntMap::iterator it = in_map.begin(); it != in_map.end(); ++it) {
    in_str[it->second] = it->first;
  }

  int32* cld_total = new int32[cld_n];
  memset(cld_total, 0, cld_n * sizeof(int32));
  int32* cld_matches = new int32[cld_n];
  memset(cld_matches, 0, cld_n * sizeof(int32));
  string* cld_str = new string[cld_n];
  for (StringIntMap::iterator it = cld_map.begin(); it != cld_map.end(); ++it) {
    cld_str[it->second] = it->first;
  }

  for (StringIntMap::iterator it = in_cld_map.begin();
        it != in_cld_map.end(); ++it) {
    string in_cld = it->first;
    int32 in_cld_int = it->second;
    //VERYTEMP
    //printf("%s[%d] = %d\n", in_cld.c_str(), in_cld_int, in_cld_count[in_cld_int]);

    // Decompose it all
    int under_pos = in_cld.find("_");
    string in_langscript = in_cld.substr(0, under_pos);
    string cld_langscript = in_cld.substr(under_pos + 1);
    int32 in_int = MapToSmallInt(in_langscript, &in_map, &next_in_map);
    int32 cld_int = MapToSmallInt(cld_langscript, &cld_map, &next_cld_map);

    Language in_lang = GetLanguageFromNumberOrName(in_langscript.c_str());
    ULScript in_lscript = GetLScriptFromNumberOrName(in_langscript.c_str());
    Language cld_lang = GetLanguageFromNumberOrName(cld_langscript.c_str());
    ULScript cld_lscript = GetLScriptFromNumberOrName(cld_langscript.c_str());

    bool match = CarefulMatch(in_langscript.c_str(),
                              in_lang, in_lscript, cld_lang, cld_lscript);

    //VERYTEMP
    //printf("%s-%s %s-%s #=%d %d %d %s\n",
    //       ExtLanguageCode(in_lang), UnicodeLScriptCode(in_lscript),
    //       ExtLanguageCode(cld_lang), UnicodeLScriptCode(cld_lscript),
    //       in_cld_count[in_cld_int], in_int, cld_int, match ? "match" : "!=");

    in_total[in_int] += in_cld_count[in_cld_int];
    cld_total[cld_int] += in_cld_count[in_cld_int];
    if (match) {
      in_matches[in_int] += in_cld_count[in_cld_int];
      cld_matches[cld_int] += in_cld_count[in_cld_int];
    }
  }

  int32 total = 0;
  int32 match_total = 0;
  for (int i = 0; i < cld_n; ++i) {
    printf("Precision: %s %d/%d = %6.4f\n",
           cld_str[i].c_str(), cld_matches[i], cld_total[i],
           cld_total[i] == 0 ? 0.0 : (cld_matches[i] * 1.0) / cld_total[i]);
    total += cld_total[i];
    match_total += cld_matches[i];
  }
  printf("Precision: %s %d/%d = %6.4f\n",
         "TOTAL", match_total, total,
         total == 0 ? 0.0 : (match_total * 1.0) / total);

  total = 0;
  match_total = 0;
  for (int i = 0; i < in_n; ++i) {
    printf("Recall: %s %d/%d = %6.4f\n",
           in_str[i].c_str(), in_matches[i], in_total[i],
           in_total[i] == 0 ? 0.0 : (in_matches[i] * 1.0) / in_total[i]);
    total += in_total[i];
    match_total += in_matches[i];
  }
  printf("Recall: %s %d/%d = %6.4f\n",
         "TOTAL", match_total, total,
         total == 0 ? 0.0 : (match_total * 1.0) / total);

#endif
}

bool SkipMe(char c) {
  if (static_cast<uint8>(c) <= '9') {return true;}
  return false;
}

// Remove any trailing digits/spaces (possible mapreduce counts)
// Return length
int Trim(char* buffer) {
  int buffer_len = strlen(buffer);
  while (SkipMe(buffer[buffer_len - 1])) {--buffer_len;}
  buffer[buffer_len] = '\0';
  return buffer_len;
}

void LangDetLinesOfFile(int flags, bool get_vector, const char* fname) {
  FILE* fin = fopen(fname, "rb");
  if (fin == NULL) {
    fprintf(stderr, "Did not open %s\n", fname);
    return;
  }

  // Expecting
  // Samp af-Latn /afr/ word tot skuldig bevind volgens die wet, in...
  char buffer[kMaxBuffer];
  while (ReadLine(fin, buffer, kMaxBuffer)) {
    if (IsComment(buffer)) {continue;}

    int buffer_len = Trim(buffer);

    string buffer_str(buffer, buffer_len);
    string lang_script;
    string tld;

    // Get lang-script
    GetStatedLangScript(buffer_str, &lang_script, &tld);
    Language in_lang = GetLanguageFromName(lang_script.c_str());
    ULScript in_lscript = GetULScriptFromName(lang_script.c_str());

    // Get Text; skip over any prefix fields
    int pos = GetTextBeginPos(buffer_str);
    if (pos == string::npos) {continue;}

    const char* src = buffer_str.data() + pos;
    int src_len = buffer_str.size() - pos;

    if (src_len < FLAGS_minsize) {continue;}    // Skip if too short

    // Detect language in one line of UTF-8
    bool is_plain_text = false;
    const char* tldhint = "";
    Encoding enchint = UNKNOWN_ENCODING;
    Language langhint = UNKNOWN_LANGUAGE;
    // Full-blown flag-bit and hints interface
    bool allow_extended_lang = true;
    // Caller initializes flags
    Language plus_one = UNKNOWN_LANGUAGE;

    Language language3[3];
    int percent3[3];
    double normalized_score3[3];
    ResultChunkVector resultchunkvector;
    int text_bytes;
    bool is_reliable;

    // Detected language biased summary (biased against English)
    Language summary_lang = UNKNOWN_LANGUAGE;

    // Identify the expected value
    fprintf(stderr, "Samp %s ", lang_script.c_str());
    flags |= kCLDFlagQuiet;

    CLDHints cldhints = {NULL, tldhint, enchint, langhint};

    summary_lang = DetectLanguageSummaryV2(
                      src, src_len,
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

#if 0
    if (FLAGS_noext) {
      summary_lang = DetectLanguageSummary(
                            src, src_len,
                            is_plain_text,
                            language3,
                            percent3,
                            &text_bytes,
                            &is_reliable);
    } else {
      summary_lang = ExtDetectLanguageSummary(
                            src, src_len,
                            is_plain_text,
                            language3,
                            percent3,
                            &text_bytes,
                            &is_reliable);
    }
#endif
    if (get_vector) {
      DumpResultChunkVector(stderr, src, &resultchunkvector);
    }

    if (!is_reliable) {summary_lang = UNKNOWN_LANGUAGE;}

    RecordCLDResult(buffer, lang_script.c_str(),
                    in_lang, in_lscript,
                    summary_lang, in_lscript);
  }

  fclose(fin);
}




int main (int argc, char *argv[])
{
  int flags = 0;
  bool get_vector = false;

  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--scoreasquads") == 0) {flags |= kCLDFlagScoreAsQuads;}
    if (strcmp(argv[i], "--html") == 0) {flags |= kCLDFlagHtml;}
    if (strcmp(argv[i], "--cr") == 0) {flags |= kCLDFlagCr;}
    if (strcmp(argv[i], "--verbose") == 0) {flags |= kCLDFlagVerbose;}
    if (strcmp(argv[i], "--vector") == 0) {get_vector = true;}
  }

  if (FLAGS_cld2_html) {
    // Begin HTML file
    fprintf(stderr, "<html><meta charset=\"UTF-8\"><body>\n");
    fprintf(stderr, "<style media=\"print\" type=\"text/css\"> "
                    ":root { -webkit-print-color-adjust: exact; } </style>\n");
    fprintf(stderr, "<span style=\"font-size: 7pt\">\n");
  }


  InitResult();
  for (int i = 1; i < argc; ++i) {
    if (argv[i][0] != '-') {
      const char* fname = argv[i];
      fprintf(stderr, "file = %s<br><br>\n", fname ? fname : "stdin");
      LangDetLinesOfFile(flags, get_vector, fname);
    }
  }
  FinishResult();

  if (FLAGS_cld2_html) {
    fprintf(stderr, "\n</span></body></html><br>");
  }

  return 0;
}

}       // End namespace CLD2


int main(int argc, char *argv[]) {
  return CLD2::main(argc, argv);
}

