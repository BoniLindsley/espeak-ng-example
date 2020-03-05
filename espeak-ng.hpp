#pragma once

// External dependencies.
#include <espeak-ng/espeak_ng.h>

// Standard C++ libraries.
#include <stdexcept>

// Standard C libraries.
#include <cassert>
#include <cstdio>

/** \brief RAII wrappers for eSpeak NG functions.
 *
 *  While wrapping the entire C API of eSpeak NG
 *  in a C++ API may be possible, it is unnecessary.
 *  The aim of interfaces in this namespace
 *  is to provide resource safety and reduce common scaffolding.
 */
namespace espeak_ng {

/** \brief Throw an exception if `code_to_check` indicates an error.
 *
 *  \param code_to_check
 *  A return value from some eSpeak NG functions.
 *  \param current_error_context
 *  Extra information related to the error if not `nullptr`.
 *  The context is _not_ cleared before `throw`-ing.
 *  \exception std::runtime_error
 *  If an error is detected.
 *
 *  If there is an error,
 *  the message corresponding to the error is written to `stderr`.
 *  An exception is then thrown after the message is written.
 *
 *  \par Usage
 *  It is provided for use with `espeak_ng::error_context`
 *  to simplify error handling.
 *  ```cpp
 *  {
 *    // When this context goes out of scope,
 *    // the underlying resources will be released automatically.
 *    // This includes if an error is thrown, thereby guarantee clean-up.
 *    espeak_ng::error_context initialisation_error;
 *
 *    // This class provides implicit conversion
 *    // to `espeak_ng_ERROR_CONTEXT*` for convenience
 *    // to be used with eSpeak NG C API.
 *    espeak_ng::throw_if_not_ok(
 *        espeak_ng_Initialize(initialisation_error),
 *        *initialisation_error);
 *
 *    // This is not called if there is an error.
 *    // but clean-up of the error context is guaranteed by destructor
 *    // even in case of a `throw`-n exception.
 *    espeak_ng_Terminate();
 *  }
 *  ```

 */
void throw_if_not_ok(
    espeak_ng_STATUS code_to_check,
    espeak_ng_ERROR_CONTEXT current_error_context = nullptr) {
  if (code_to_check != ENS_OK) {
    espeak_ng_PrintStatusCodeMessage(
        code_to_check, stderr, current_error_context);
    throw std::runtime_error("eSpeak-NG error");
  }
}

/** \brief Provides RAII for eSpeak NG error context.
 *
 *  \par Purpose
 *  Some eSpeak NG functions
 *  contain an `OUT` `espeak_ng_ERROR_CONTEXT*` parameter
 *  to provide additional information when they encounter an error.
 *  And if an error is encountered,
 *  the argument given to the function must release the resources
 *  allocated for the `espeak_ng_ERROR_CONTEXT`
 *  using the function `espeak_ng_ClearErrorContext`.
 *  This class ensures that the function will be called.
 *
 *  \par Usage
 *  Since instances of this class is expected to be used with the C API,
 *  the interface of this class is modelled after
 *  a pointer to `espeak_ng_ERROR_CONTEXT`
 *  instead of the error context itself.
 *  This means that, for most usage, instances of this class can be seen
 *  as a managed `espeak_ng_ERROR_CONTEXT*` pointer.
 *  To provide a limited pointer interface,
 *  an implicit conversion
 *  with `error_context::operator espeak_ng_ERROR_CONTEXT*()`
 *  gives a pointer to the stored `espeak_ng_ERROR_CONTEXT`.
 *  To provide an interface expected of a pointer,
 *  this class also provides the indirection `error_context::operator*`
 *  to "dereference" the implicit conversion.
 *  That is, it returns
 *  the stored `espeak_ng_ERROR_CONTEXT` `error_context::data` value.
 *  ```cpp
 *  {
 *    // When this context goes out of scope,
 *    // the underlying resources will be released automatically.
 *    espeak_ng::error_context initialisation_error;
 *
 *    // This class provides implicit conversion
 *    // to `espeak_ng_ERROR_CONTEXT*` for convenience
 *    // to be used with eSpeak NG C API.
 *    auto code_to_check = espeak_ng_Initialize(initialisation_error);
 *
 *    if (code_to_check != ENS_OK) {
 *      // Dereferenceing with the indirection operator
 *      // dereferences the implicit conversion,
 *      // thereby retrieving the underlying stored context.
 *      espeak_ng_PrintStatusCodeMessage(
 *          code_to_check, stderr, *initialisation_error);
 *
 *      // When this excepion is `throw`-n,
 *      // stack unwinding will call destructor of this class
 *      // and release resources.
 *      throw;
 *    }
 *    espeak_ng_Terminate();
 *  }
 *  ```
 *
 *  \see
 *  The function `espeak_ng::throw_if_not_ok` for
 *  possible further simplication.
  */
class error_context {
public:

  /** \brief The stored eSpeak NG error context.
   *
   *  Normal usage should not need to directly access this member.
   *  There is implicit conversion
   *  with `error_context::operator espeak_ng_ERROR_CONTEXT*()`
   *  to directly use `error_context` with eSpeak NG C API,
   *  and indirection `error_context::operator*`
   *  to access this `error_context::data`.
   */
  espeak_ng_ERROR_CONTEXT data{nullptr};

  /** \brief Ensures resources allocated to the context is released. */
  ~error_context() {
    // It is safe to give `nullptr` and a pointer to `nullptr`
    // to the release function.
    espeak_ng_ClearErrorContext(&data);
  }

  /** \brief Implicit conversion to pointer to `error_context::data`.
   *
   *  This is provided for convenience
   *  to be used in `OUT` parameters in the C API of eSpeak NG.
   */
  operator espeak_ng_ERROR_CONTEXT*() {
    assert(data == nullptr);
    return &data;
  }

  /** \brief Returns the `error_context::data` value itself.
   *
   *  Ownership of the error context remains with `this`.
   *  The returned value should be treated as a raw pointer
   *  (and it is a raw pointer in implementation)
   *  in the sense that no ownership is transferred.
   */
  espeak_ng_ERROR_CONTEXT operator*() { return data; }
};

/** \brief Provides RAII for eSpeak NG initialisation.
 *
 *  \par Purpose
 *  Usage of the C API of eSpeak NG begins with an `espeak_ng_Initialize`
 *  and ends with an `espeak_ng_Terminate`.
 *  This class ensures that the terminate function is called
 *  if the initialisation was successful.
 *
 *  \par Usage
 *  Note that eSpeak NG must be given a path
 *  to its data before initialisation.
 *  Otherwise initialisation fails.
 *  ```cpp
 *  {
 *    espeak_ng_InitializePath(nullptr);
 *    espeak_ng::service service;
 *  } // Calls `espeak_ng_Terminate` unless initialisation failed.
 *  ```
 */
class service {
public:
  /** \brief Calls `espeak_ng_Initialize`.
   *
   *  \exception std::runtime_error
   *  If initialisation fails.
   */
  service() {
    error_context initialisation_error;
    throw_if_not_ok(
        espeak_ng_Initialize(initialisation_error),
        *initialisation_error);
  }

  /** \brief Calls `espeak_ng_Terminate`. */
  ~service() {
    // The termination function returns an error code,
    // but the function always succeeds.
    // So there is no need to check it.
    espeak_ng_Terminate();
  }
};

} // namespace espeak_ng
