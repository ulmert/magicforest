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
 * File: mforest.cpp
 *
 * Sample player
 *
 */

#include "mforest.hpp"
#include "notemap.h"
#include "samplebank.h"

static MForest mforest;

void OSC_INIT(uint32_t platform, uint32_t api)
{
  (void)platform;
  (void)api;

  initSamples();

  Sample *sample = &samples[SAMPLE_IDX_DRONE];

  MForest::Voice *droneVoice = &mforest.voices[VOICE_IDX_DRONE];

  droneVoice->sampleIdx = sample->offset;
  droneVoice->sampleIdxEnd = sample->len + droneVoice->sampleIdx;

  droneVoice->sampleLen = sample->len;
  droneVoice->sampleLoopIdx = sample->loopIdx;

  droneVoice->sampleStep = sample->rate / (k_samplerate * 2.f);
  droneVoice->sampleCnt = 0;

  droneVoice->loopInf = sample->loopInf;
  droneVoice->gain = 0.5;

  droneVoice->isPlaying = true;

}

void OSC_CYCLE(const user_osc_param_t * const params, int32_t *yn, const uint32_t frames)
{
  MForest::Params &p = mforest.params;
  MForest::Voice *voices = &mforest.voices[0];
  {
 
    if (mforest.state.trigSample) {
      mforest.state.trigSample = TRIGSAMPLE_NONE;

      uint16_t map = notemap[(params->pitch)>>8];
      
      uint16_t i = N_SAMPLES;
      while (i != 0) {
        i--;
        if ((map & (1 << i))) { 
          uint8_t rand = (uint8_t)osc_rand();

          if ((rand > 195) || !(samples[i].flags & SAMPLE_FLAG_RANDOM)) {
            Sample *sample = &samples[i];
            uint8_t voiceIdx = sample->voiceIdx;
            if (voiceIdx == DYNAMIC_SAMPLE_IDX) {
              uint8_t t = N_DYNAMIC_VOICES;
              while (t != 0) {
                t--;
                if (!voices[t].isPlaying && !voices[t].delayedTrigTimer) {
                  voiceIdx = t;
                  break;     
                }
              }
            }
            if (voiceIdx != DYNAMIC_SAMPLE_IDX) {
              MForest::Voice &voice = voices[voiceIdx];
              if (sample->flags & SAMPLE_FLAG_LEAD) {
                voice.sampleInc = 0;
                voice.LFOenv = 0;
              } 

              if (sample->flags & SAMPLE_FLAG_HP) {
                voice.hpf.mCoeffs.setFOHP(0.785 * 1.25 + (0.785 * osc_white()));
              }

              voice.sampleIdx = sample->offset;
              voice.sampleIdxEnd = sample->len + voice.sampleIdx;

              voice.sampleLen = sample->len;
              voice.sampleLoopIdx = sample->loopIdx;
              voice.sampleCnt = 0;

              if (sample->flags & SAMPLE_FLAG_DELAY) {
                voice.delayedTrigTimer = rand;
                voice.isPlaying = false;
              } else {
                voice.isPlaying = true;
              }

              voice.loopInf = sample->loopInf;
              voice.gain = *voice.gainGroup;
          
              if (sample->flags & SAMPLE_FLAG_LIM_PITCH) {
                voice.sampleStep = ((sample->rate - ((osc_white() * 0.5) + 0.5) * sample->pitchRange)) / k_samplerate ;
              } else {
                voice.sampleStep = ((sample->rate) * (1.0 - *voice.tuneGroup)) / k_samplerate;
              }

              if (sample->flags & SAMPLE_FLAG_RAND_PITCH) {
                if (rand > 250) {
                  voice.sampleStep *= 0.75;
                } else if (rand > 240) {
                  voice.sampleStep *= 0.5;
                } else if (rand > 200) {
                  voice.sampleStep *= 1.5;
                }
              }

              if (sample->flags & SAMPLE_FLAG_NOTE_PITCH) {
                voice.sampleStep *= osc_notehzf((params->pitch)>>8) / 220.0;
              } 
              if (rand & 1) {
                voice.sampleStep *= 0.75;
              }

              voice.repeat = 0;
              if ((!voice.loopInf) && sample->flags & SAMPLE_FLAG_REPEAT) {
                voice.repeat = rand & 0x3;
              }
        
              if (voice.sampleLoopIdx == 0) {
                voice.decayDur = (1.0 / voice.sampleStep) * voice.sampleLen;
              } else {
                voice.decayDur = voice.sampleLen;
              }
        
              voice.sampleIdxStart = voice.sampleIdx;
            }
          }
        }
      }
    }
  }

  uint8_t i = MAX_VOICES;
  while (i != 0) {
    i--;
    MForest::Voice &voice = voices[i];
    if (voice.isPlaying) {
      if (voice.sampleCnt >= voice.decayDur && !voice.loopInf) {
        voice.isPlaying = false;
      } else if (voice.sampleIdx >= voice.sampleIdxEnd) {
        if (voice.sampleLoopIdx != 0) {
          voice.sampleIdx = voice.sampleLoopIdx + (voice.sampleIdx - (float)voice.sampleIdxEnd);
        } else {
          voice.isPlaying = false;
        }
      } 
    } else if (voice.delayedTrigTimer != 0) {
      voice.delayedTrigTimer--;
      if (voice.delayedTrigTimer == 0) {
        voice.isPlaying = true;
      }
    }

    if (voice.repeat != 0 && voice.sampleCnt >= voice.decayDur) {
      voice.sampleIdx = voice.sampleIdxStart;
      voice.sampleCnt = 0;
      voice.gain *= 0.75;
      voice.isPlaying = true;   
      voice.repeat--;
      voice.hpf.mCoeffs.setFOHP(0.785 * 2 + (0.785 / 4.0 * osc_white()));
    } 
  }

  MForest::Voice &voice0 = voices[0];  
  MForest::Voice &voice1 = voices[1];  
  MForest::Voice &voiceDrum = voices[2];  
  MForest::Voice &voiceDrone = voices[3];  
  MForest::Voice &voiceLead = voices[4];

  const bool voice0_isPlaying = voice0.isPlaying;
  const bool voice1_isPlaying = voice1.isPlaying;
  const bool voiceDrum_isPlaying = voiceDrum.isPlaying;
  const bool voiceLead_isPlaying = voiceLead.isPlaying;

  voiceDrone.hpf.mCoeffs.setFOHP(0.785 / 2.0 + (0.785 / 4.0 * osc_sinf(voiceDrone.LFOenv)));
  voiceDrone.LFOenv += 0.0001; 
  if (voiceDrone.LFOenv >= 1) {
    voiceDrone.LFOenv = voiceDrone.LFOenv - 1.f;
  }
  const float droneGain = p.gainGroup_a * 0.25;

  const float leadGain = (osc_sinf(voiceLead.LFOenv) * 0.5 + 0.5) * voiceLead.gain;
  if (voiceLead_isPlaying) {
    voiceLead.LFOenv += 0.00015 * frames;
  }

  uint16_t sampleIdx;
  float fr;
  float sig;

  q31_t * __restrict y = (q31_t *)yn;
  const q31_t * y_e = y + frames;

  for (; y != y_e; ) {

    sampleIdx = (uint16_t)voiceDrone.sampleIdx;
    fr = (voiceDrone.sampleIdx - sampleIdx);
    const float sample = (((sampleBank[sampleIdx] * (1.0 - fr)) + (sampleBank[sampleIdx + 1] * fr)) / DENOMINATOR_8BIT);
    sig = (voiceDrone.hpf.process_fo(sample) + osc_white() * 0.025) * droneGain;
    voiceDrone.sampleIdx += voiceDrone.sampleStep;
    voiceDrone.sampleCnt++;

    if (voice0_isPlaying) {
      sampleIdx = (uint16_t)voice0.sampleIdx;
      fr = (voice0.sampleIdx - sampleIdx);
      const float sample = (((sampleBank[sampleIdx] * (1.0 - fr)) + (sampleBank[sampleIdx + 1] * fr)) / DENOMINATOR_8BIT);
      sig += (voice0.hpf.process_fo(sample) * (1.f - ((float)voice0.sampleCnt / (float)voice0.decayDur))) * voice0.gain;
      voice0.sampleIdx += voice0.sampleStep;
      voice0.sampleCnt++;
    }

    if (voice1_isPlaying) {
      sampleIdx = (uint16_t)voice1.sampleIdx;
      fr = (voice1.sampleIdx - sampleIdx);
      const float sample = (((sampleBank[sampleIdx] * (1.0 - fr)) + (sampleBank[sampleIdx + 1] * fr)) / DENOMINATOR_8BIT);
      sig += (voice1.hpf.process_fo(sample) * (1.f - ((float)voice1.sampleCnt / (float)voice1.decayDur))) * voice1.gain;
      voice1.sampleIdx += voice1.sampleStep;
      voice1.sampleCnt++;
    }

    if (voiceDrum_isPlaying) {
      sampleIdx = (uint16_t)voiceDrum.sampleIdx;
      fr = (voiceDrum.sampleIdx - sampleIdx);
      const float sample = (((sampleBank[sampleIdx] * (1.0 - fr)) + (sampleBank[sampleIdx + 1] * fr)) / DENOMINATOR_8BIT);
      sig += (sample * (1.f - ((float)voiceDrum.sampleCnt / (float)voiceDrum.decayDur))) * voiceDrum.gain;
      voiceDrum.sampleIdx += voiceDrum.sampleStep;
      voiceDrum.sampleCnt++;
    }

    if (voiceLead_isPlaying) {
      sampleIdx = (uint16_t)(voiceLead.sampleIdx);
      fr = (voiceLead.sampleIdx - sampleIdx);
      float sample = (((sampleBank[sampleIdx] * (1.0 - fr)) + (sampleBank[sampleIdx + 1] * fr)) / DENOMINATOR_8BIT);
      sig += (sample * (1.f - ((float)voiceLead.sampleCnt / (float)voiceLead.decayDur))) * leadGain;
      voiceLead.sampleIdx += voiceLead.sampleStep;
      voiceLead.sampleCnt += voiceLead.sampleInc;
    }
    *(y++) = f32_to_q31(sig);
  }
}

void OSC_NOTEON(const user_osc_param_t * const params) 
{
  mforest.state.trigSample = TRIGSAMPLE_NOTEON;
}

void OSC_NOTEOFF(const user_osc_param_t * const params)
{
  mforest.voices[LEAD_VOICE_IDX].sampleInc = 1;
}

void OSC_PARAM(uint16_t index, uint16_t value)
{ 
  MForest::Params &p = mforest.params;

  switch (index) {
    case k_user_osc_param_id1: 
      p.tuneGroup_b = value / 100.f;
      if (p.tuneGroup_b < 0.1) {
        p.tuneGroup_b = 0.1;
      }
      break;
  
    case k_user_osc_param_id2:
      p.tuneGroup_a = value / 100.f;
      if (p.tuneGroup_a < 0.1) {
        p.tuneGroup_a = 0.1;
      }
      break;
    
    case k_user_osc_param_shape:
      p.gainGroup_b = param_val_to_f32(value);
      break;

    case k_user_osc_param_shiftshape:
      p.gainGroup_a = param_val_to_f32(value);
      break;

    default:
      break;
  }
}