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
// Unit test compact language detector, CLD2
//  Compile with -Davoid_utf8_string_constants if your compiler cannot
//  handle UTF-8 string constants
//

#include <stdio.h>
#include <string.h>

#include "../public/compact_lang_det.h"
#include "../public/encodings.h"
#include "unittest_data.h"


namespace CLD2 {

// Test strings.
const char* kTeststr_en =
  "confiscation of goods is assigned as the penalty part most of the courts "
  "consist of members and when it is necessary to bring public cases before a "
  "jury of members two courts combine for the purpose the most important cases "
  "of all are brought jurors or";


typedef struct {
  Language lang;
  const char* text;
} TestPair;


static const TestPair kTestPair[] = {
// A simple case to begin
  {ENGLISH, kTeststr_en},

// 20 languages recognized via Unicode script
  {ARMENIAN, kTeststr_hy_Armn},
  {CHEROKEE, kTeststr_chr_Cher},
  {DHIVEHI, kTeststr_dv_Thaa},
  {GEORGIAN, kTeststr_ka_Geor},
  {GREEK, kTeststr_el_Grek},
  {GUJARATI, kTeststr_gu_Gujr},
  {INUKTITUT, kTeststr_iu_Cans},
  {KANNADA, kTeststr_kn_Knda},
  {KHMER, kTeststr_km_Khmr},
  {LAOTHIAN, kTeststr_lo_Laoo},
  {LIMBU, kTeststr_lif_Limb},
  {MALAYALAM, kTeststr_ml_Mlym},
  {ORIYA, kTeststr_or_Orya},
  {PUNJABI, kTeststr_pa_Guru},
  {SINHALESE, kTeststr_si_Sinh},
  {SYRIAC, kTeststr_syr_Syrc},
  {TAGALOG, kTeststr_tl_Tglg},      // Also in quadgram list below
  {TAMIL, kTeststr_ta_Taml},
  {TELUGU, kTeststr_te_Telu},
  {THAI, kTeststr_th_Thai},

// 4 languages regognized via single letters
  {CHINESE, kTeststr_zh_Hans},
  {CHINESE_T, kTeststr_zh_Hant},
  {JAPANESE, kTeststr_ja_Hani},
  {KOREAN, kTeststr_ko_Hani},

// 60 languages recognized via combinations of four letters
  {AFRIKAANS, kTeststr_af_Latn},
  {ALBANIAN, kTeststr_sq_Latn},
  {ARABIC, kTeststr_ar_Arab},
  {AZERBAIJANI, kTeststr_az_Latn},
  {BASQUE, kTeststr_eu_Latn},
  {BELARUSIAN, kTeststr_be_Cyrl},
  {BENGALI, kTeststr_bn_Beng},      // No Assamese in subset
  {BIHARI, kTeststr_bh_Deva},
  {BULGARIAN, kTeststr_bg_Cyrl},
  {CATALAN, kTeststr_ca_Latn},
  {CEBUANO, kTeststr_ceb_Latn},
  {CROATIAN, kTeststr_hr_Latn},
  {CZECH, kTeststr_cs_Latn},
  {DANISH, kTeststr_da_Latn},
  {DUTCH, kTeststr_nl_Latn},
  {ENGLISH, kTeststr_en_Latn},
  {ESTONIAN, kTeststr_et_Latn},
  {FINNISH, kTeststr_fi_Latn},
  {FRENCH, kTeststr_fr_Latn},
  {GALICIAN, kTeststr_gl_Latn},
  {GANDA, kTeststr_lg_Latn},
  {GERMAN, kTeststr_de_Latn},
  {HAITIAN_CREOLE, kTeststr_ht_Latn},
  {HEBREW, kTeststr_iw_Hebr},
  {HINDI, kTeststr_hi_Deva},
  {HMONG, kTeststr_blu_Latn},
  {HUNGARIAN, kTeststr_hu_Latn},
  {ICELANDIC, kTeststr_is_Latn},
  {INDONESIAN, kTeststr_id_Latn},
  {IRISH, kTeststr_ga_Latn},
  {ITALIAN, kTeststr_it_Latn},
  {JAVANESE, kTeststr_jw_Latn},
  {KINYARWANDA, kTeststr_rw_Latn},
  {LATVIAN, kTeststr_lv_Latn},
  {LITHUANIAN, kTeststr_lt_Latn},
  {MACEDONIAN, kTeststr_mk_Cyrl},
  {MALAY, kTeststr_ms_Latn},
  {MALTESE, kTeststr_mt_Latn},
  {MARATHI, kTeststr_mr_Deva},
  {NEPALI, kTeststr_ne_Deva},
  {NORWEGIAN, kTeststr_no_Latn},
  {PERSIAN, kTeststr_fa_Arab},
  {POLISH, kTeststr_pl_Latn},
  {PORTUGUESE, kTeststr_pt_Latn},
  {ROMANIAN, kTeststr_ro_Latn},
  {ROMANIAN, kTeststr_ro_Cyrl},
  {RUSSIAN, kTeststr_ru_Cyrl},
  {SCOTS_GAELIC,  kTeststr_gd_Latn},
  {SERBIAN, kTeststr_sr_Cyrl},
  {SERBIAN, kTeststr_sr_Latn},
  {SLOVAK, kTeststr_sk_Latn},
  {SLOVENIAN, kTeststr_sl_Latn},
  {SPANISH,  kTeststr_es_Latn},
  {SWAHILI, kTeststr_sw_Latn},
  {SWEDISH, kTeststr_sv_Latn},
  {TAGALOG, kTeststr_tl_Latn},
  {TURKISH, kTeststr_tr_Latn},
  {UKRAINIAN, kTeststr_uk_Cyrl},
  {URDU, kTeststr_ur_Arab},
  {VIETNAMESE, kTeststr_vi_Latn},
  {WELSH, kTeststr_cy_Latn},
  {YIDDISH, kTeststr_yi_Hebr},

// 2 statistically-close languages
  {INDONESIAN, kTeststr_id_close},
  {MALAY, kTeststr_ms_close},

// Simple intermixed French/English text
  {FRENCH, kTeststr_fr_en_Latn},

// Cross-check the main quadgram table build date
// Change the expected language each time it is rebuilt
  {WELSH, kTeststr_version},

  {UNKNOWN_LANGUAGE, NULL},     // Must be last
};


bool OneTest(int flags, bool get_vector,
             Language lang_expected, const char* buffer, int buffer_length) {
  bool is_plain_text = true;
  const char* tldhint = "";
  const Encoding enchint = UNKNOWN_ENCODING;
  const Language langhint = UNKNOWN_LANGUAGE;
  const CLDHints cldhints = {NULL, tldhint, enchint, langhint};
  Language language3[3];
  int percent3[3];
  double normalized_score3[3];
  ResultChunkVector resultchunkvector;
  int text_bytes;
  bool is_reliable;

  Language lang_detected = ExtDetectLanguageSummary(
                          buffer,
                          buffer_length,
                          is_plain_text,
                          &cldhints,
                          flags,
                          language3,
                          percent3,
                          normalized_score3,
                          get_vector ? &resultchunkvector : NULL,
                          &text_bytes,
                          &is_reliable);
// expose DumpExtLang DumpLanguages

  bool ok = (lang_detected == lang_expected);

  if (!ok) {
    if ((flags & kCLDFlagHtml) != 0) {
      fprintf(stderr, "*** Wrong result. expected %s, detected %s<br>\n",
              LanguageName(lang_expected), LanguageName(lang_detected));
    }
    fprintf(stdout, "*** Wrong result. expected %s, detected %s\n",
            LanguageName(lang_expected), LanguageName(lang_detected));
    fprintf(stdout, "%s\n\n", buffer);
  }

  if (get_vector) {
    DumpResultChunkVector(stderr, buffer, &resultchunkvector);
  }

#if 0
  DumpExtLang(flags, summary_lang, language3, percent3, normalized_score3,
              text_bytes, is_reliable, n);

  if ((flags & kCLDFlagHtml) != 0) {
    DumpLanguages(summary_lang,
                  language3, percent3, text_bytes, is_reliable, n);
  }

  fprintf(stdout, "  SummaryLanguage %s%s at %u of %d, %s\n",
         LanguageName(summary_lang),
         is_reliable ? "" : "(un-reliable)",
         bytes_consumed,
         n,
         argv[1]);
#endif

  return ok;
}

void InitHtmlOut(int flags) {
#if 1
  if ((flags & kCLDFlagHtml) != 0) {
    // Begin HTML file
    fprintf(stderr, "<html><meta charset=\"UTF-8\"><body>\n");
    // Encourage browsers to print background colors
    fprintf(stderr, "<style media=\"print\" type=\"text/css\"> "
                    ":root { -webkit-print-color-adjust: exact; } </style>\n");
    fprintf(stderr, "<span style=\"font-size: 7pt\">\n");
    fprintf(stderr, "file = %s<br>\n", "cld2_unittest");
  }
#endif
}

void FinishHtmlOut(int flags) {
#if 1
  if ((flags & kCLDFlagHtml) != 0) {
    fprintf(stderr, "\n</span></body></html>\n");
  }
#endif
}

int RunTests (int flags, bool get_vector) {
  fprintf(stdout, "CLD2 version: %s\n", CLD2::DetectLanguageVersion());
  InitHtmlOut(flags);
  bool any_fail = false;
  int i = 0;
  while (kTestPair[i].text != NULL) {
    Language lang_expected = kTestPair[i].lang;
    const char* buffer = kTestPair[i].text;
    int buffer_length = strlen(buffer);
    bool ok = OneTest(flags, get_vector, lang_expected, buffer, buffer_length);
    any_fail |= (!ok);
    ++i;
  }
  if (any_fail) {
    fprintf(stderr, "FAIL\n");
    fprintf(stdout, "FAIL\n");
  } else {
    fprintf(stderr, "PASS\n");
    fprintf(stdout, "PASS\n");
  }

  FinishHtmlOut(flags);
  return 0;
}

}       // End namespace CLD2

int main(int argc, char** argv) {
  // Get command-line flags
  int flags = 0;
  bool get_vector = false;
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--html") == 0) {flags |= CLD2::kCLDFlagHtml;}
    if (strcmp(argv[i], "--cr") == 0) {flags |= CLD2::kCLDFlagCr;}
    if (strcmp(argv[i], "--verbose") == 0) {flags |= CLD2::kCLDFlagVerbose;}
    if (strcmp(argv[i], "--quiet") == 0) {flags |= CLD2::kCLDFlagQuiet;}
    if (strcmp(argv[i], "--echo") == 0) {flags |= CLD2::kCLDFlagEcho;}
    if (strcmp(argv[i], "--vector") == 0) {get_vector = true;}
  }

  return CLD2::RunTests(flags, get_vector);
}

