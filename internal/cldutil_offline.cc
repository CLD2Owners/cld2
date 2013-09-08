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

#include "cldutil_offline.h"
#include "tote.h"
#include <string>

static const int kMinCJKUTF8CharBytes = 3;

//------------------------------------------------------------------------------
// Offline: used by mapreduce or table construction
//------------------------------------------------------------------------------

namespace CLD2 {

// BIGRAM, QUADGRAM, OCTAGRAM score one => tote
// Input: 4-byte entry of 3 language numbers and one probability subscript, plus
//  an accumulator tote. (language 0 means unused entry)
// Output: running sums in tote updated
void ProcessProbV2Tote(uint32 probs, Tote* tote) {
  uint8 prob123 = (probs >> 0) & 0xff;
  const uint8* prob123_entry = LgProb2TblEntry(prob123);

  uint8 top1 = (probs >> 8) & 0xff;
  if (top1 > 0) {tote->Add(top1, LgProb3(prob123_entry, 0));}
  uint8 top2 = (probs >> 16) & 0xff;
  if (top2 > 0) {tote->Add(top2, LgProb3(prob123_entry, 1));}
  uint8 top3 = (probs >> 24) & 0xff;
  if (top3 > 0) {tote->Add(top3, LgProb3(prob123_entry, 2));}
}

// Advances src, decrements len
uint32 GetNextLangprob(ULScriptRType rtype,
                       const CLD2TableSummary* wrt_unigram_obj,
                       const CLD2TableSummary* wrt_quadgram_obj,
                       const char** isrc, int* isrclen) {
  // fprintf(stderr, "GetNextLangprob '%s' %d<br>\n", *isrc, *isrclen);
  if (*isrclen <= 0) {return 0;}

  // Find one quadgram
  const char* src = *isrc;
  const char* srclimit = src + *isrclen;
  if (*src == ' ') {++src;}
  const char* src_end = src;
  src_end += kAdvanceOneCharButSpace[(uint8)src_end[0]];
  src_end += kAdvanceOneCharButSpace[(uint8)src_end[0]];
  const char* src_mid = src_end;
  src_end += kAdvanceOneCharButSpace[(uint8)src_end[0]];
  src_end += kAdvanceOneCharButSpace[(uint8)src_end[0]];
  int len = src_end - src;
  // Hash the quadgram
  uint32 quadhash = QuadHashV2(src, len);
  uint32 probs = QuadHashV3Lookup4(wrt_quadgram_obj, quadhash);
  int indirect_subscr = probs & ~wrt_quadgram_obj->kCLDTableKeyMask;
  uint32 langprob;
  if (indirect_subscr < static_cast<int>(wrt_quadgram_obj->kCLDTableSizeOne)) {
    // Up to three languages at indirect
    langprob = wrt_quadgram_obj->kCLDTableInd[indirect_subscr];
  } else {
    // Up to six languages at start + 2 * (indirect - start)
    indirect_subscr += (indirect_subscr - wrt_quadgram_obj->kCLDTableSizeOne);
    langprob = wrt_quadgram_obj->kCLDTableInd[indirect_subscr];
  }
  // Advance: all the way past word if at end-of-word, else 2 chars
  if (src_end[0] == ' ') {
    src = src_end;
  } else {
    src = src_mid;
  }
  if (src < srclimit) {
    src += kAdvanceOneCharSpaceVowel[(uint8)src[0]];
  } else {
    // Advancing by 4/8/16 can overshoot, but we are about to exit anyway
    src = srclimit;
  }
  int quadadvance = src - *isrc;
  *isrc = src;
  *isrclen -= quadadvance;
  return langprob;
}


// Find top two langs and scores for one word; underpins delta tables
void DoWordScore(const char* isrc, int srclen, ULScript ulscript,
                 const CLD2TableSummary* wrt_unigram_obj,
                 const CLD2TableSummary* wrt_quadgram_obj,
                 Language* lang1, int* score1,
                 Language* lang2, int* score2) {
  ULScriptRType rtype = ULScriptRecognitionType(ulscript);

  Tote word_tote;
  const char* src = isrc;
  int len = srclen;
  uint32 langprob;

  // Advances src, decrements len
  langprob = GetNextLangprob(rtype, wrt_unigram_obj, wrt_quadgram_obj,
                             &src, &len);
  ProcessProbV2Tote(langprob, &word_tote);

  // Advances src, decrements len
  langprob = GetNextLangprob(rtype, wrt_unigram_obj, wrt_quadgram_obj,
                             &src, &len);
  ProcessProbV2Tote(langprob, &word_tote);

  int key3[3];
  word_tote.CurrentTopThreeKeys(key3);
  *lang1 = FromPerScriptNumber(ulscript, key3[0]);
  *lang2 = FromPerScriptNumber(ulscript, key3[1]);
  *score1 = word_tote.GetScore(key3[0]);
  *score2 = word_tote.GetScore(key3[1]);
}

// Routines to store 3 or 5 log probabilities in a single byte.
// Resolution/range = 2**1 to 2**12
//------------------------------------------------------------------------------

// For constructing tables
// Given a vector of 3 probabilities 1..12, find subscript of best table match.
// Minimizes RMS error
// Brute-force version
uint8 FindBestProb3Match(const uint8* prob3) {
  int minsubscr = 0;
  int minrmserr = 9999;
  for (int i = 0; i < kLgProbV2TblSize; ++i) {
    int rmserr = 0;
    for (int j = 0; j < 3; ++j) {
      // If target prob is zero, item is unused, so no errterm
      if (prob3[j] > 0) {
        int errterm = prob3[j] - LgProb3(LgProb2TblEntry(i), j);
        rmserr += (errterm * errterm);
      }
    }
    if (minrmserr > rmserr) {
      minrmserr = rmserr;
      minsubscr = i;
    }
  }
  return static_cast<uint8>(minsubscr);
};

// Not sure who calls this...
// Return the probability for given language, or 0
int GetProb(Language lang, uint32 probs) {
  int prob123 = (probs >> 0) & 0xff;
  const uint8* prob123_entry = LgProb2TblEntry(prob123);

  int ilang = PerScriptNumber(ULScript_Latin, lang);
  int top1 = (probs >> 8) & 0xff;
  if (ilang == top1) {return LgProb3(prob123_entry, 0);}
  int top2 = (probs >> 16) & 0xff;
  if (ilang == top2) {return LgProb3(prob123_entry, 1);}
  int top3 = (probs >> 16) & 0xff;
  if (ilang == top3) {return LgProb3(prob123_entry, 2);}
  return 0;
}


// Converts a unigram prob/lang byte into an approximate prob/lang triple
// Just keeps the largest value.
// Now unused.
uint32 ApproxProb3(int propval) {
   return 0;
}


// Take three packed languages and three probabilities 1..12 and put into uint32
// For offline construction of tables
uint32 ProbPackV2(uint8* plang3, uint8* prob3) {
  uint32 retval;
  // If < 3 entries, pack as top, 0, second, else pack as top, second, third
  // This allows FindBestProb3Match to always find a perfect match for < 3
  if (plang3[2] == 0) {
    // Swap [2] and [3]
    uint8 temp = plang3[2]; plang3[2] = plang3[1]; plang3[1] = temp;
    temp = prob3[2]; prob3[2] = prob3[1]; prob3[1] = temp;
  }
  retval = (plang3[2] << 24) |
    (plang3[1] << 16) |
    (plang3[0] << 8) |
    (FindBestProb3Match(prob3));
  return retval;
}

// Take uint32 and unpack into three packed languages and three probabilities
// For runtime use of tables
void ProbUnpackV2(uint32 prob, uint8* plang3, uint8* prob3) {
  plang3[0] = (prob >> 8) & 0xff;
  plang3[1] = (prob >> 16) & 0xff;
  plang3[2] = (prob >> 24) & 0xff;

  int prob123 = (prob >> 0) & 0xff;
  const uint8* prob123_entry = LgProb2TblEntry(prob123);
  prob3[0] = LgProb3(prob123_entry, 0);
  prob3[1] = LgProb3(prob123_entry, 1);
  prob3[2] = LgProb3(prob123_entry, 2);
}

}       // End namespace CLD2



