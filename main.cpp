// Local dependencies.
#include "boni.hpp"
#include "espeak-ng.hpp"
/** \brief Ask SDL2 to not change `main` into a macro. */
#define SDL_MAIN_HANDLED
#include "sdl2.hpp"

// External dependencies.
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_timer.h>
#include <espeak-ng/espeak_ng.h>

// Standard C++ libraries.
#include <stdexcept>
#include <string>

// Standard C libraries.
#include <cassert>
#include <cstdio>
#include <cstdlib>

/** \param wav
 *  Speech data produced (since last callback?)
 *  It is `nullptr` if the synthesis has been completed (and paused?)
 *  \param numsamples
 *  Length, possibly zero, of the array pointed to by `wav`.
 *  \param events
 *  A `type`-`0`-terminated array of `espeak_EVENT` items
 *  indicating word and sentences events,
 *  the occurance of mark and audio elements within the text.
 *  \return `0` if synthesis should continue, `1` to abort.
 */
int SynthCallback(short* wav, int numsamples, espeak_EVENT* events) {
  assert(events != nullptr); // Pre-condition.

  for (; events->type != espeakEVENT_LIST_TERMINATED; ++events) {
    auto& event = *events;
    switch (event.type) {
    case espeakEVENT_LIST_TERMINATED:
    case espeakEVENT_WORD:
    case espeakEVENT_SENTENCE:
    case espeakEVENT_MARK:
    case espeakEVENT_PLAY:
    case espeakEVENT_END:
    case espeakEVENT_MSG_TERMINATED:
    case espeakEVENT_PHONEME:
    case espeakEVENT_SAMPLERATE:
      break;
    }
  }
  auto& event = *events;
  if (wav != nullptr && numsamples != 0) {
    // Cannot do anything with the data without the audio device.
    auto audio_device =
        static_cast<sdl2::audio_device*>(event.user_data);
    if (audio_device) {
      sdl2::throw_if(
          0 != SDL_QueueAudio(*audio_device, wav, numsamples * 2));
    }
  }
  return 0;
}

int main() {
  std::fprintf(stderr, "Starting eSpeak NG service.\n");
  // Let eSpeak NG know where to find voice data.
  // Use a default location by passing a `nullptr`.
  // It must be given to eSpeak NG before initialisation.
  espeak_ng_InitializePath(nullptr);
  espeak_ng::service service;

  {
    std::fprintf(stderr, "Initialising output.\n");
    auto const status = espeak_ng_InitializeOutput(
        // Must be synchronous to use synthesis callback.
        // Playing audio instead would bypass the callback.
        ENOUTPUT_MODE_SYNCHRONOUS, 0, nullptr);
    espeak_ng::throw_if_not_ok(status);
  }

  std::fprintf(stderr, "Getting sample rate.\n");
  auto sample_rate = espeak_ng_GetSampleRate();

  std::fprintf(stderr, "Starting SDL2 audio service.\n");
  sdl2::service audio_service{SDL_INIT_AUDIO};

  sdl2::audio_device audio_device;
  {
    SDL_AudioSpec required_audio_spec;
    {
      SDL_zero(required_audio_spec);
      required_audio_spec.freq = sample_rate;
      required_audio_spec.format = AUDIO_S16LSB;
      required_audio_spec.channels = 1u;
      required_audio_spec.samples = 4096u;
    }
    audio_device = sdl2::audio_device{SDL_OpenAudioDevice(
        nullptr, 0, &required_audio_spec, nullptr, 0)};
    sdl2::throw_if(nullptr == audio_device.get());
  }

  std::fprintf(stderr, "Setting synthesis callback.\n");
  espeak_SetSynthCallback(SynthCallback);

  {
    std::fprintf(stderr, "Start synthesis.\n");
    auto const text_to_speak = std::string{"Hello world."};
    auto const status = espeak_ng_Synthesize(
        text_to_speak.c_str(), text_to_speak.size() + 1, 0,
        POS_CHARACTER, text_to_speak.size(), espeakCHARS_AUTO, 0,
        &audio_device);
    SDL_PauseAudioDevice(audio_device, 0);
    espeak_ng::throw_if_not_ok(status);
  }

  {
    std::fprintf(stderr, "Syncrhonising.\n");
    auto const status = espeak_ng_Synchronize();
    espeak_ng::throw_if_not_ok(status);
  }
  std::fprintf(stderr, "Exiting.\n");
  SDL_Delay(1500);
}
