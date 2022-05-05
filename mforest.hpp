#pragma once
/*
    BSD 3-Clause License

    Copyright (c) 2022, Jacob Ulmert
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
 *  File: mforest.hpp
 *
 *  Sample player
 *
 */

#include "userosc.h"
#include "fx_api.h"
#include "biquad.hpp"

#define MAX_VOICES 5

struct MForest {
  
  struct Params {
    float tuneGroup_a;
    float tuneGroup_b;

    float gainGroup_a;
    float gainGroup_b;

    Params(void) : 
      tuneGroup_a(1.0), 
      tuneGroup_b(1.0),
      gainGroup_a(1.0), 
      gainGroup_b(1.0)
    { 
    }
  };


  struct Voice {
    float sampleIdx;
    float sampleIdxStart;

    float sampleStep;
    float sampleLoopIdx;

    uint16_t sampleLen;
    uint16_t sampleIdxEnd;

    uint16_t sampleCnt;
    uint16_t decayDur;
    uint16_t sampleInc;

    float repeatGainDec;

    float gain;

    float *gainGroup;
    float *tuneGroup;
  
    uint8_t repeat;
    float repeatDur;

    float LFOenv;

    bool isPlaying;

    bool loopInf;

    uint32_t delayedTrigTimer;
 
    dsp::BiQuad hpf;
  
    Voice(void) : 
      isPlaying(false), loopInf(false), delayedTrigTimer(0)
    {
    }
  };

  #define TRIGSAMPLE_NONE 0
  #define TRIGSAMPLE_NOTEON 1
  #define TRIGSAMPLE_RANDOM (1 << 1)

  struct State {
    uint8_t trigSample;

    float droneLFOenv;
 
    State(void) : 
      trigSample(TRIGSAMPLE_NONE), droneLFOenv(0)
    {
    }
  };

  MForest(void) {
    init();
  }

  void init(void) {
    state = State();
    params = Params();

    uint8_t i = MAX_VOICES;
    while(i != 0) {
      i--;
      voices[i] = Voice();
    }

    voices[0].gainGroup = &params.gainGroup_a;
    voices[1].gainGroup = &params.gainGroup_a;
    voices[2].gainGroup = &params.gainGroup_b;
    voices[3].gainGroup = &params.gainGroup_a;
    voices[4].gainGroup = &params.gainGroup_b;

    voices[0].tuneGroup = &params.tuneGroup_a;
    voices[1].tuneGroup = &params.tuneGroup_a;
    voices[2].tuneGroup = &params.tuneGroup_b;
    voices[3].tuneGroup = &params.tuneGroup_a;
    voices[4].tuneGroup = &params.tuneGroup_b;
  }

  State state;
  Params params;
  Voice voices[MAX_VOICES];

};
