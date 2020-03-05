#pragma once

// Local dependencies.
#include "boni.hpp"

// External dependencies.
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>

// Standard C++ libraries.
#include <stdexcept>

// Standard C libraries.
#include <cassert>
#include <cstdint>
#include <cstdio>

/** \brief RAII wrappers for SDL2 functions.
 *
 *  While wrapping the entire C API of SDL2
 *  in a C++ API may be possible, it is unnecessary.
 *  The aim of interfaces in this namespace
 *  is to provide resource safety and reduce common scaffolding.
 *
 *  ```cpp
 *  {
 *    sdl2::service audio_service{SDL_INIT_AUDIO};
 *    sdl2::audio_device primary_audio_device;
 *    {
 *      // Put the specification into a tightest scope.
 *      SDL_AudioSpec required_audio_spec;
 *      {
 *        SDL_zero(required_audio_spec);
 *        required_audio_spec.freq = 22050;
 *        required_audio_spec.format = AUDIO_S16LSB;
 *        required_audio_spec.channels = 1u;
 *        required_audio_spec.samples = 4096u;
 *      }
 *      primary_audio_device = sdl2::audio_device{SDL_OpenAudioDevice(
 *          nullptr, 0, &required_audio_spec, nullptr, 0)};
        sdl2::throw_if(nullptr == primary_audio_device.get());
 *    }
 *
 *    // Do things with the device.
 *    // For example, this unpauses the audio device.
 *    SDL_PauseAudioDevice(primary_audio_device, 0);
 *
 *    // Closes the audio device automatically.
 *    // And then `SDL_quit`-s.
 *  }
 *  ```
 */
namespace sdl2 {

/** \brief Throw `std:runtime_error`
 *  if the given `is_throwing` condition is `true`.
 *
 *  If `is_throwing` indicates an error,
 *  the message corresponding to the error is written to `stderr`.
 *  An exception is then thrown after the message is written.
 *  Note that the output stream is not flushed explicitly.
 */
void throw_if(const bool is_throwing) {
  if (is_throwing) {
    std::fprintf(stderr, "%s\n", SDL_GetError());
    throw std::runtime_error("SDL2 error");
  }
}

/** \brief Provides RAII for SDL2 initialisation.
 *
 *  Usage of the C API of SDL2 begins with an `SDL_Init`
 *  and ends with an `SDL_Quit`.
 *  This class ensures that the quit function is called
 *  if the initialisation was successful.
 */
class service {
public:

  /** \brief Initialises the given subsystem.
   *
   *  If `SDL_MAIN_HANDLED` is defined when this header is included,
   *  calls `SDL_SetMainReady`.
   *  In either case, also calls `SDL_Init` with the given flags.
   */
  service(uint32_t subsystem_flags) {
#ifdef SDL_MAIN_HANDLED
    SDL_SetMainReady();
#endif
    throw_if(0 != SDL_Init(subsystem_flags));
  }

  /** \brief Calls `SDL_Quit`. */
  ~service() { SDL_Quit(); }
};

/** \brief Callable taking an audio device ID and closes the device.
 *
 *  This is an implementation detail
 *  for making RAII for `sdl2::audio_device`.
 */
using audio_device_deleter =
    boni::nullable_deleter<SDL_AudioDeviceID, SDL_CloseAudioDevice>;

/** \brief Represents SDL audio devices.
 *
 *  Closes the managed device when going out of scope.
 */
using audio_device = boni::auto_handle<audio_device_deleter>;

} // namespace sdl2
