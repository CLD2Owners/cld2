#!/bin/sh
#
#  Copyright 2014 Google Inc. All Rights Reserved.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http:# www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.

if [ -z "${CFLAGS}" -a -z "${CXXFLAGS}" -a -z "${CPPFLAGS}" ]; then
  echo "Warning: None of CFLAGS, CXXFLAGS or CPPFLAGS is set; you probably should enable some options." 1>&2
fi
if [ -n "${CFLAGS}" ]; then
  echo "CFLAGS=${CFLAGS}"
fi
if [ -n "${CXXFLAGS}" ]; then
  echo "CXXFLAGS=${CXXFLAGS}"
fi
if [ -n "${CPPFLAGS}" ]; then
  echo "CPPFLAGS=${CPPFLAGS}"
fi

g++ $CFLAGS $CPPFLAGS $CXXFLAGS compact_lang_det_test.cc \
  cldutil.cc cldutil_shared.cc compact_lang_det.cc  compact_lang_det_hint_code.cc \
  compact_lang_det_impl.cc  debug.cc fixunicodevalue.cc \
  generated_entities.cc  generated_language.cc generated_ulscript.cc  \
  getonescriptspan.cc lang_script.cc offsetmap.cc  scoreonescriptspan.cc \
  tote.cc utf8statetable.cc  \
  cld_generated_cjk_uni_prop_80.cc cld2_generated_cjk_compatible.cc  \
  cld_generated_cjk_delta_bi_4.cc generated_distinct_bi_0.cc  \
  cld2_generated_quadchrome_2.cc cld2_generated_deltaoctachrome.cc \
  cld2_generated_distinctoctachrome.cc  cld_generated_score_quad_octa_2.cc  \
  -o compact_lang_det_test_chrome_2 $LDFLAGS
echo "  compact_lang_det_test_chrome_2 compiled"

g++ $CFLAGS $CPPFLAGS $CXXFLAGS compact_lang_det_test.cc \
  cldutil.cc cldutil_shared.cc compact_lang_det.cc  compact_lang_det_hint_code.cc \
  compact_lang_det_impl.cc  debug.cc fixunicodevalue.cc \
  generated_entities.cc  generated_language.cc generated_ulscript.cc  \
  getonescriptspan.cc lang_script.cc offsetmap.cc  scoreonescriptspan.cc \
  tote.cc utf8statetable.cc  \
  cld_generated_cjk_uni_prop_80.cc cld2_generated_cjk_compatible.cc  \
  cld_generated_cjk_delta_bi_4.cc generated_distinct_bi_0.cc  \
  cld2_generated_quadchrome_16.cc cld2_generated_deltaoctachrome.cc \
  cld2_generated_distinctoctachrome.cc  cld_generated_score_quad_octa_2.cc  \
  -o compact_lang_det_test_chrome_16 $LDFLAGS
echo "  compact_lang_det_test_chrome_16 compiled"


g++ $CFLAGS $CPPFLAGS $CXXFLAGS cld2_unittest.cc \
  cldutil.cc cldutil_shared.cc compact_lang_det.cc  compact_lang_det_hint_code.cc \
  compact_lang_det_impl.cc  debug.cc fixunicodevalue.cc \
  generated_entities.cc  generated_language.cc generated_ulscript.cc  \
  getonescriptspan.cc lang_script.cc offsetmap.cc  scoreonescriptspan.cc \
  tote.cc utf8statetable.cc  \
  cld_generated_cjk_uni_prop_80.cc cld2_generated_cjk_compatible.cc  \
  cld_generated_cjk_delta_bi_4.cc generated_distinct_bi_0.cc  \
  cld2_generated_quadchrome_2.cc cld2_generated_deltaoctachrome.cc \
  cld2_generated_distinctoctachrome.cc  cld_generated_score_quad_octa_2.cc  \
  -o cld2_unittest_chrome_2 $LDFLAGS
echo "  cld2_unittest_chrome_2 compiled"

g++ $CFLAGS $CPPFLAGS $CXXFLAGS -Davoid_utf8_string_constants cld2_unittest.cc \
  cldutil.cc cldutil_shared.cc compact_lang_det.cc  compact_lang_det_hint_code.cc \
  compact_lang_det_impl.cc  debug.cc fixunicodevalue.cc \
  generated_entities.cc  generated_language.cc generated_ulscript.cc  \
  getonescriptspan.cc lang_script.cc offsetmap.cc  scoreonescriptspan.cc \
  tote.cc utf8statetable.cc  \
  cld_generated_cjk_uni_prop_80.cc cld2_generated_cjk_compatible.cc  \
  cld_generated_cjk_delta_bi_4.cc generated_distinct_bi_0.cc  \
  cld2_generated_quadchrome_2.cc cld2_generated_deltaoctachrome.cc \
  cld2_generated_distinctoctachrome.cc  cld_generated_score_quad_octa_2.cc  \
  -o cld2_unittest_avoid_chrome_2 $LDFLAGS
echo "  cld2_unittest_avoid_chrome_2 compiled"




