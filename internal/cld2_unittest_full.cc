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
// A couple of simple cases to begin
  {ENGLISH, kTeststr_en},
  //{KASHMIRI, kTeststr_ks}, // Full test is below

// 21 languages recognized via Unicode script
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
  {MONGOLIAN,  kTeststr_mn_Mong},   // Also in quadgram list below
  {ORIYA, kTeststr_or_Orya},
  {PUNJABI, kTeststr_pa_Guru},
  {SINHALESE, kTeststr_si_Sinh},
  {SYRIAC, kTeststr_syr_Syrc},
  {TAGALOG, kTeststr_tl_Tglg},      // Also in quadgram list below
  {TAMIL, kTeststr_ta_Taml},
  {TELUGU, kTeststr_te_Telu},
  {THAI, kTeststr_th_Thai},

  {X_Buginese,  kTeststr_xx_Bugi},  // Not on offically-recognized list
  {X_Gothic,  kTeststr_xx_Goth},    // Not on offically-recognized list

// 4 languages regognized via single letters
  {CHINESE, kTeststr_zh_Hans},
  {CHINESE_T, kTeststr_zh_Hant},
  {JAPANESE, kTeststr_ja_Hani},
  {KOREAN, kTeststr_ko_Hani},

// ~150 language-script combinations recognized via combinations of four letters
  {ABKHAZIAN,  kTeststr_ab_Cyrl},
  {AFAR,  kTeststr_aa_Latn},
  {AFRIKAANS,  kTeststr_af_Latn},
  {AKAN,  kTeststr_ak_Latn},
  {ALBANIAN,  kTeststr_sq_Latn},
  {AMHARIC,  kTeststr_am_Ethi},
  {ARABIC,  kTeststr_ar_Arab},
  {ASSAMESE,  kTeststr_as_Beng},
  {AYMARA,  kTeststr_ay_Latn},
  // Not trained {AZERBAIJANI,  kTeststr_az_Arab},
  {AZERBAIJANI,  kTeststr_az_Latn},
  {BASHKIR,  kTeststr_ba_Cyrl},
  {BASQUE,  kTeststr_eu_Latn},
  {BELARUSIAN,  kTeststr_be_Cyrl},
  {BENGALI,  kTeststr_bn_Beng},
  {BIHARI,  kTeststr_bh_Deva},
  {BISLAMA,  kTeststr_bi_Latn},
  // Added 2014.01.22 bs-Latn
  {BOSNIAN,  kTeststr_bs_Latn},

  {BRETON,  kTeststr_br_Latn},
  {BULGARIAN,  kTeststr_bg_Cyrl},
  // Not trained {BURMESE,  kTeststr_my_Latn},   // Myanmar
  {BURMESE,  kTeststr_my_Mymr},   // Myanmar
  {CATALAN,  kTeststr_ca_Latn},
  {CEBUANO,  kTeststr_ceb_Latn},
  {CORSICAN,  kTeststr_co_Latn},
  {CROATIAN,  kTeststr_hr_Latn},
  {CZECH,  kTeststr_cs_Latn},

  {DANISH,  kTeststr_da_Latn},
  {DUTCH,  kTeststr_nl_Latn},
  {DZONGKHA,  kTeststr_dz_Tibt},
  {ENGLISH,  kTeststr_en_Latn},
  {ESPERANTO,  kTeststr_eo_Latn},
  {ESTONIAN,  kTeststr_et_Latn},
  // Not trained {EWE,  kTeststr_ee_Latn},
  {FAROESE,  kTeststr_fo_Latn},
  {FIJIAN,  kTeststr_fj_Latn},
  {FINNISH,  kTeststr_fi_Latn},
  {FRENCH,  kTeststr_fr_Latn},
  {FRISIAN,  kTeststr_fy_Latn},

  // Not trained {GA,  kTeststr_gaa_Latn},
  {GALICIAN,  kTeststr_gl_Latn},
  {GANDA,  kTeststr_lg_Latn},
  {GERMAN,  kTeststr_de_Latn},
  {GREENLANDIC,  kTeststr_kl_Latn},
  {GUARANI,  kTeststr_gn_Latn},
  {HAITIAN_CREOLE,  kTeststr_ht_Latn},
  {HAUSA,  kTeststr_ha_Latn},
  {HAWAIIAN,  kTeststr_haw_Latn},
  {HEBREW,  kTeststr_iw_Hebr},
  {HINDI,  kTeststr_hi_Deva},
  {HMONG,  kTeststr_blu_Latn},
  {HUNGARIAN,  kTeststr_hu_Latn},
  {ICELANDIC,  kTeststr_is_Latn},
  {IGBO,  kTeststr_ig_Latn},
  {INDONESIAN,  kTeststr_id_Latn},
  {INTERLINGUA,  kTeststr_ia_Latn},
  {INTERLINGUE,  kTeststr_ie_Latn},
  {INUPIAK,  kTeststr_ik_Latn},
  {IRISH,  kTeststr_ga_Latn},
  {ITALIAN,  kTeststr_it_Latn},

  {JAVANESE,  kTeststr_jw_Latn},
  {KASHMIRI,  kTeststr_ks_Arab},
  // Not trained {KASHMIRI,  kTeststr_ks_Deva},
  {KAZAKH,  kTeststr_kk_Arab},
  {KAZAKH,  kTeststr_kk_Cyrl},
  // Not trained {KAZAKH,  kTeststr_kk_Latn},
  {KHASI,  kTeststr_kha_Latn},
  {KINYARWANDA,  kTeststr_rw_Latn},
  // Not trained {KRIO,  kTeststr_kri_Latn},
  {KURDISH,  kTeststr_ku_Arab},
  // Not trained {KURDISH,  kTeststr_ku_Latn},
  {KYRGYZ,  kTeststr_ky_Arab},
  {KYRGYZ,  kTeststr_ky_Cyrl},
  {LATIN,  kTeststr_la_Latn},
  {LATVIAN,  kTeststr_lv_Latn},
  {LINGALA,  kTeststr_ln_Latn},
  {LITHUANIAN,  kTeststr_lt_Latn},
  // Not trained {LOZI,  kTeststr_loz_Latn},
  // Not trained {LUBA_LULUA,  kTeststr_lua_Latn},
  // Not trained {LUO_KENYA_AND_TANZANIA,  kTeststr_luo_Latn},
  {LUXEMBOURGISH,  kTeststr_lb_Latn},

  {MACEDONIAN,  kTeststr_mk_Cyrl},
  {MALAGASY,  kTeststr_mg_Latn},
  {MALAY,  kTeststr_ms_Latn},
  {MALAY,  kTeststr_ms_Latn2},
  {MALTESE,  kTeststr_mt_Latn},
  {MANX,  kTeststr_gv_Latn},
  {MAORI,  kTeststr_mi_Latn},
  {MARATHI,  kTeststr_mr_Deva},
  {MAURITIAN_CREOLE,  kTeststr_mfe_Latn},
  {MONGOLIAN,  kTeststr_mn_Cyrl},
  // Not trained {MONTENEGRIN,  kTeststr_sr_ME_Latn},   // Not recognized as distinct from Croatian/Serbian
  {NAURU,  kTeststr_na_Latn},
  // Added 2014.01.22 nr-Latn
  {NDEBELE,  kTeststr_nr_Latn},

  {NEPALI,  kTeststr_ne_Deva},
  // Not trained {NEWARI,  kTeststr_new_Latn},
  {NORWEGIAN,  kTeststr_no_Latn},
  {NORWEGIAN_N,  kTeststr_nn_Latn},
  {NYANJA,  kTeststr_ny_Latn},
  {OCCITAN,  kTeststr_oc_Latn},
  {OROMO,  kTeststr_om_Latn},
  // Not trained {OSSETIAN,  kTeststr_os_Latn},

  // Not trained {PAMPANGA,  kTeststr_pam_Latn},
  {PASHTO,  kTeststr_ps_Arab},
  {PEDI,  kTeststr_nso_Latn},
  {PERSIAN,  kTeststr_fa_Arab},
  {POLISH,  kTeststr_pl_Latn},
  {PORTUGUESE,  kTeststr_pt_Latn},
  {QUECHUA,  kTeststr_qu_Latn},
  // Not trained  {RAJASTHANI,  kTeststr_raj_Latn},
  {RHAETO_ROMANCE,  kTeststr_rm_Latn},
  {ROMANIAN,  kTeststr_ro_Cyrl},
  {ROMANIAN,  kTeststr_ro_Latn},
  {RUNDI,  kTeststr_rn_Latn},
  {RUSSIAN,  kTeststr_ru_Cyrl},

  {SAMOAN,  kTeststr_sm_Latn},
  {SANGO,  kTeststr_sg_Latn},
  {SANSKRIT,  kTeststr_sa_Deva},
  {SANSKRIT,  kTeststr_sa_Latn},
  {SCOTS,  kTeststr_sco_Latn},
  {SCOTS_GAELIC,  kTeststr_gd_Latn},
  {SERBIAN,  kTeststr_sr_Cyrl},
  {SERBIAN,  kTeststr_sr_Latn},
  {SESELWA,  kTeststr_crs_Latn},
  {SESOTHO,  kTeststr_st_Latn},
  {SHONA,  kTeststr_sn_Latn},
  {SINDHI,  kTeststr_sd_Arab},
  {SISWANT,  kTeststr_ss_Latn},
  {SLOVAK,  kTeststr_sk_Latn},
  {SLOVENIAN,  kTeststr_sl_Latn},
  {SOMALI,  kTeststr_so_Latn},
  {SPANISH,  kTeststr_es_Latn},
  {SUNDANESE,  kTeststr_su_Latn},
  {SWAHILI,  kTeststr_sw_Latn},
  {SWEDISH,  kTeststr_sv_Latn},
  {TAGALOG,  kTeststr_tl_Latn},
  // Not trained {TAJIK,  kTeststr_tg_Arab},
  {TAJIK,  kTeststr_tg_Cyrl},
  {TATAR,  kTeststr_tt_Cyrl},
  {TATAR,  kTeststr_tt_Latn},
  {TIBETAN,  kTeststr_bo_Tibt},
  {TIGRINYA,  kTeststr_ti_Ethi},
  {TONGA,  kTeststr_to_Latn},
  {TSONGA,  kTeststr_ts_Latn},
  {TSWANA,  kTeststr_tn_Latn},
  // Not trained {TUMBUKA,  kTeststr_tum_Latn},
  {TURKISH,  kTeststr_tr_Latn},
  {TURKMEN,  kTeststr_tk_Cyrl},
  {TURKMEN,  kTeststr_tk_Latn},
  {/*TWI*/ AKAN,  kTeststr_tw_Latn},   // TWI Recognized as  AKAN
  {UIGHUR,  kTeststr_ug_Arab},
  {UIGHUR,  kTeststr_ug_Cyrl},
  // Not trained {UIGHUR,  kTeststr_ug_Latn},
  {UKRAINIAN,  kTeststr_uk_Cyrl},
  {URDU,  kTeststr_ur_Arab},
  {UZBEK,  kTeststr_uz_Arab},
  {UZBEK,  kTeststr_uz_Cyrl},
  {UZBEK,  kTeststr_uz_Latn},

  {VENDA,  kTeststr_ve_Latn},
  {VIETNAMESE,  kTeststr_vi_Latn},
  {VOLAPUK,  kTeststr_vo_Latn},
  {WARAY_PHILIPPINES,  kTeststr_war_Latn},
  {WELSH,  kTeststr_cy_Latn},
  {WOLOF,  kTeststr_wo_Latn},
  {XHOSA,  kTeststr_xh_Latn},
  {X_KLINGON,  kTeststr_tlh_Latn},
  {X_PIG_LATIN,  kTeststr_zzp_Latn},
  {YIDDISH,  kTeststr_yi_Hebr},
  {YORUBA,  kTeststr_yo_Latn},
  // Not trained {ZHUANG,  kTeststr_za_Hani},
  {ZHUANG,  kTeststr_za_Latn},
  {ZULU,  kTeststr_zu_Latn},


// 2 statistically-close languages
  {INDONESIAN, kTeststr_id_close},
  {MALAY, kTeststr_ms_close},

// Simple intermixed French/English text
  {FRENCH, kTeststr_fr_en_Latn},

// Cross-check the main quadgram table build date
// Change the expected language each time it is rebuilt
  //{SLOVENIAN, kTeststr_version},  // 2013.07.20
  {ICELANDIC, kTeststr_version},  // 2014.02.04

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
    fprintf(stderr, "file = %s<br>\n", "cld2_unittest_full");
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
  return any_fail ? 1 : 0;
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

