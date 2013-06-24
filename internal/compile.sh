g++ -O2 -m64  compact_lang_det_test.cc \
  cldutil.cc cldutil_shared.cc compact_lang_det.cc  compact_lang_det_hint_code.cc \
  compact_lang_det_impl.cc  debug.cc fixunicodevalue.cc \
  generated_entities.cc  generated_language.cc generated_ulscript.cc  \
  getonescriptspan.cc lang_script.cc offsetmap.cc  scoreonescriptspan.cc \
  tote.cc utf8statetable.cc  \
  cld_generated_cjk_uni_prop_80.cc cld2_generated_cjk_compatible.cc  \
  cld_generated_cjk_delta_bi_4.cc generated_distinct_bi_0.cc  \
  cld2_generated_quadchrome0604.cc cld2_generated_deltaoctachrome0614.cc \
  cld2_generated_distinctoctachrome0604.cc  cld_generated_score_quad_octa_1024_256.cc  \
  -o compact_lang_det_test_chrome0614
echo "  compact_lang_det_test_chrome0614 compiled"

g++ -O2 -m64  cld2_unittest.cc \
  cldutil.cc cldutil_shared.cc compact_lang_det.cc  compact_lang_det_hint_code.cc \
  compact_lang_det_impl.cc  debug.cc fixunicodevalue.cc \
  generated_entities.cc  generated_language.cc generated_ulscript.cc  \
  getonescriptspan.cc lang_script.cc offsetmap.cc  scoreonescriptspan.cc \
  tote.cc utf8statetable.cc  \
  cld_generated_cjk_uni_prop_80.cc cld2_generated_cjk_compatible.cc  \
  cld_generated_cjk_delta_bi_4.cc generated_distinct_bi_0.cc  \
  cld2_generated_quadchrome0604.cc cld2_generated_deltaoctachrome0614.cc \
  cld2_generated_distinctoctachrome0604.cc  cld_generated_score_quad_octa_1024_256.cc  \
  -o cld2_unittest
echo "  cld2_unittest compiled"

g++ -O2 -m64  -Davoid_utf8_string_constants cld2_unittest.cc \
  cldutil.cc cldutil_shared.cc compact_lang_det.cc  compact_lang_det_hint_code.cc \
  compact_lang_det_impl.cc  debug.cc fixunicodevalue.cc \
  generated_entities.cc  generated_language.cc generated_ulscript.cc  \
  getonescriptspan.cc lang_script.cc offsetmap.cc  scoreonescriptspan.cc \
  tote.cc utf8statetable.cc  \
  cld_generated_cjk_uni_prop_80.cc cld2_generated_cjk_compatible.cc  \
  cld_generated_cjk_delta_bi_4.cc generated_distinct_bi_0.cc  \
  cld2_generated_quadchrome0604.cc cld2_generated_deltaoctachrome0614.cc \
  cld2_generated_distinctoctachrome0604.cc  cld_generated_score_quad_octa_1024_256.cc  \
  -o cld2_unittest_avoid
echo "  cld2_unittest -Davoid_utf8_string_constants compiled"

