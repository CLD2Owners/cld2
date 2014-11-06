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
set -e
rm -f compact_lang_det_test_chrome
rm -f cld2_unittest
rm -f cld2_unittest_avoid
rm -f libcld2.so
rm -f libcld2_full.so
rm -f compact_lang_det_test_full
rm -f cld2_unittest_full
rm -f cld2_unittest_full_avoid
rm -f cld2_dynamic_data_tool
rm -f cld2_data.bin
rm -f compact_lang_det_dynamic_test_chrome
rm -f cld2_dynamic_unittest
rm -f libcld2_dynamic.so
rm -f compact_lang_det_test_chrome_16
rm -f compact_lang_det_test_chrome_2
rm -f cld2_unittest_avoid_chrome_2
rm -f cld2_unittest_chrome_2
