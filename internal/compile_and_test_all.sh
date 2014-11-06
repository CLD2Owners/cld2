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
trap 'echo "FAILED!" && exit 1' INT TERM EXIT

# ----------------------------------------------------------------------------
echo "--> [1 of 7] Invoking: compile.sh..."
./compile.sh
echo "--> [2 of 7] Testing results of compile.sh..."
echo "--> compact_lang_det_test_chrome_2... "
echo "this is some english text" | ./compact_lang_det_test_chrome_2
echo "--> compact_lang_det_test_chrome_16... "
echo "this is some english text" | ./compact_lang_det_test_chrome_16
echo "--> cld2_unittest_chrome_2... "
./cld2_unittest_chrome_2 > /dev/null
echo "--> cld2_unittest_avoid_chrome_2... "
./cld2_unittest_avoid_chrome_2 > /dev/null

# ----------------------------------------------------------------------------
echo "--> [3 of 7] Invoking: compile_libs.sh..."
./compile_libs.sh

# ----------------------------------------------------------------------------
echo "--> [4 of 7] Invoking: compile_full.sh..."
./compile_full.sh
echo "--> [5 of 7] Testing results of compile_full.sh..."
echo "--> compact_lang_det_test_full... "
echo "this is some english text" | ./compact_lang_det_test_full
echo "--> cld2_unittest_full... "
./cld2_unittest_full > /dev/null
echo "--> cld2_unittest_full_avoid... "
./cld2_unittest_full_avoid > /dev/null

# ----------------------------------------------------------------------------
echo "--> [6 of 7] Invoking: compile_dynamic.sh..."
./compile_dynamic.sh
echo "--> [6 of 7] Dumping dynamic data to cld2_data.bin..."
./cld2_dynamic_data_tool --dump cld2_data.bin
echo "--> [6 of 7] Verifying dynamic data in cld2_data.bin..."
./cld2_dynamic_data_tool --verify cld2_data.bin
echo "--> [7 of 7] Testing results of compile_dynamic.sh..."
echo "--> compact_lang_det_dynamic_test_chrome... "
echo "this is some english text" | ./compact_lang_det_dynamic_test_chrome --data-file cld2_data.bin
echo "--> cld2_dynamic_unittest... "
./cld2_dynamic_unittest --data-file cld2_data.bin > /dev/null

trap - INT TERM EXIT
echo "All libraries compiled and all tests passed!"
