#pragma once

// Standard library.
#include <array>
#include <memory>
#include <string>

// C standard library.
#include <cstddef>
#include <cstdio>
#include <cstdlib>

/** \brief Contains convenience RAII wrappers.
 *
 *  The main aim of these wrappers are to reduce code duplication,
 *  in the sense of copy-paste-edit,
 *  and the likelihood of resoure leak.
 *  They are created as needed to provided minimal functionalities,
 *  with no attempt to make them as general as possible.
 */
namespace boni {

/** \brief Wraps a type so that it is nullable.
 *
 *  \tparam handle_type
 *  The type that is to be used as a "pointer".
 *  Effectively, it is the type of the data to be stored
 *  in instances of this class.
 *  \tparam null_value
 *  The value to be stored
 *  when the "pointer" represents a "null" pointer.
 *
 *  The type `T` given to `std::unique_ptr<T, Deleter>`
 *  must be a `NullablePointer`.
 *  However, commonly used handle types such as `int`
 *  do not satisfy this requirement.
 *  This class wraps such types so that they do.
 */
template <typename handle_type, handle_type null_value = handle_type{}>
class nullable {
public:
  /** \brief Refers to this type. */
  using self_type = nullable;

  /** \brief Initialises stored value to `null_value`.
   *
   *  A `NullablePointer` must be `DefaultConstructible
   *  and value initialises to its null value.
   */
  nullable() = default;

  /** \brief Initialises stored value to `null_value`.
   *
   * A `NullablePointer` must convert a `nullptr` to its null value.
   */
  nullable(std::nullptr_t) : nullable{} {};

  /** \brief Converts from the underlying type. For convenience.
   *
   *  This is needed so that `std::unique_ptr<nullable<T>>`
   *  can accept instances of type `T` in its constructor.
   */
  nullable(handle_type new_value) : value{new_value} {}

  /** \brief Returns whether `null_value` is stored.
   *
   *  A `NullablePointer` must return `false`
   *  if and only if its value is equivalent to its null value.
   */
  explicit operator bool() const { return value != null_value; }

  /** \brief Converts to underlying type. For convenience. */
  operator handle_type() const { return value; }

  /** \brief Compares the stored `handle_type` data.
   *
   *  A `NullablePointer` must be `EqualityComparable`
   *  and must also be comparable with `nullptr` using `==`.
   */
  friend bool operator==(self_type left, self_type right) {
    return static_cast<handle_type>(left) ==
           static_cast<handle_type>(right);
  }

  /** \brief Compares the stored `handle_type` data.
   *
   *  A `NullablePointer` must be comparable with `nullptr` using `!=`.
   */
  friend bool operator!=(self_type left, self_type right) {
    return !(left == right);
  }

  /** \brief The underlying stored data. */
  handle_type value{null_value};
};

/** \brief A deleter that accepts a non-`NullablePointer` as a handle.
 *
 *  This is similar to `boni::handle_deleter`
 *  but with `handle_type_t` being possibly not a `NullablePointer`,
 *  This is necessary, for example, when a handle is `int`.
 *
 *  \par Implementation Consideration
 *  It is possible to generalise this to `boni::unsafe_nullable_deleter`
 *  mimicking `unsafe_handle_deleter`,
 *  but I currently do not need it.
 *  It may also be possible to implement `is_nullable_pointer`
 *  and automatically wrap the `handle_type_t` when necessary
 *  in `boni::handle_deleter`,
 *  removing the need for this class altogether.
 */
template <
    typename handle_type_t, void (*destroy_function)(handle_type_t)>
class nullable_deleter {
public:
  /** \brief The `handle_type` that the `destroy_function` acts on.
   *
   *  In particular, this is the type that C API acts on.
   */
  using handle_type = handle_type_t;
  /** \brief The type to be stored by `std::unique_ptr`.
   *
   *  This is for use as `Deleter::pointer`
   *  in the `std::unique_ptr<T, Deleter>` interface.
   *  It is a `NullablePointer`
   *  and is implicitly convertible to `handle_type`.
   */
  using pointer = nullable<handle_type>;

  /** \brief Destroy the given handle.
   *
   *  That is, this callable simply forwards to `destroy_function`,
   *  but only if the argument is not "null".
   */
  void operator()(pointer pointer_to_delete) {
    if (pointer_to_delete == pointer()) {
      destroy_function(pointer_to_delete);
    }
  }
};

/** \brief Callable wrapping delete function of a handle.
 *
 *  \tparam handle_type
 *  The type of the _handle_ itself that this deleter destroys.
 *  So, `handle_type` typically correspond
 *  to `T*` in `std::unique_ptr<T>`.
 *  \tparam return_type
 *  The type returned by `destroy_function`.
 *  This information is not really needed by the class itself,
 *  but it is needed
 *  to define the template argument of `destroy_function`.
 *  It also serves as a reminder to the user
 *  that there is a return value to the `destroy_function`.
 *  \tparam destroy_function
 *  A pointer to a function responsible for releasing the resource
 *  managed by the instances of `handle_type`.
 *  It need not handle the case with input `handle_type()`
 *  which will be filtered out by this class.
 *
 *  The deleter in the template parameter of `std::unique_ptr`
 *  cannot be a function pointer.
 *  This class wraps around a function pointer to produce a deleter
 *  that is usable as a deleter for `std::unique_ptr`.
 *  An example of using this would be
 *  to [close](https://en.cppreference.com/w/cpp/io/c/fclose)-ing
 *  `FILE` handles:
 *
 *  ```cpp
 *  // Standard libraries.
 *  #include <memory>
 *  #include <string>
 *
 *  // C standard libraries.
 *  #include <cstdio>
 *  #include <cstdlib>
 *
 *  // Creating a deleter requires knowing the handle type
 *  // and the function necessary to release the managed resource.
 *  // Note that, unlike `unique_ptr`,
 *  // the template parameter is a pointer `T*` instead of `T`.
 *  using file_deleter = handle_deleter<FILE*, std::fclose>;
 *
 *  // This deleter can then be given to `std::unique_ptr`,
 *  // to automatically manage the lifetime of the window.
 *  using file = std::unique_ptr<FILE*, file_deleter>;
 *
 *  int main() {
 *    // Use the handle as it normally would be used.
 *    // It has the same interface as `std::unique_ptr`.
 *    std::string readme_filename{"README.md"};
 *    file readme_file{std::fopen(readme_filename.c_str(), "r")};
 *    if (readme_file.get() == nullptr) {
 *      std::fprintf(
 *        stderr, "Unable to open file: %s\n", readme_filename.c_str());
 *      return EXIT_FAILURE;
 *    }
 *    // Here, `fclose(readme_file.get());` is called automatically.
 *  }
 *  ```
 *
 *  This provides automatic lifetime management of `FILE`,
 *  assuming the created handle is given directly to `file`.
 *  This class is unsafe in the sense that
 *  the value returned by `destroy_function` is discarded,
 *  despite its likely purpose of indicating errors.
 */
template <
    typename handle_type_t, typename return_type,
    return_type (*destroy_function)(handle_type_t)>
class unsafe_handle_deleter {
public:
  /** \brief The `handle_type` that the `destroy_function` acts on.
   *
   *  This is for use as `Deleter::pointer = handle_type`
   *  in the `std::unique_ptr<T, Deleter>` interface.
   */
  using handle_type = handle_type_t;
  using pointer = handle_type;

  /** \brief Destroy the given handle.
   *
   *  That is, this callable simply forwards to `destroy_function`,
   *  but only if the argument is not "null".
   */
  void operator()(pointer handle_to_delete) {
    if (handle_to_delete == pointer()) {
      destroy_function(handle_to_delete);
    }
  }
};

/** \brief Callable wrapping delete function of a handle.
 *
 *  \tparam handle_type
 *  The type of the _handle_ itself that the deleter destroys.
 *  \tparam destroy_function
 *  A pointer to a function responsible for releasing the resource
 *  managed by the instances of `handle_type`.
 *
 *  See \ref unsafe_handle_deleter for slightly more information
 *  on the invterface of this class.
 *  Compared to that class,
 *  the difference lies in the lack of a return value
 *  in the `destroy_function`.
 *  This is more "safe" in the sense that there is no return value,
 *  possibly used to indicate error, to ignore.
 *
 *  The template parameter `Deleter` of `std::unique_ptr`
 *  cannot be a function pointer.
 *  This class wraps around a function pointer to produce a class
 *  that is usable as a `Deleter` for `std::unique_ptr`.
 *  An example of using this would be to
 *  [`SDL_DestroyWindow`](https://wiki.libsdl.org/SDL_DestroyWindow)
 *  whenever a `window` goes out of scope.
 *
 *  ```cpp
 *  // Dependencies.
 *  #include <SDL.h>
 *
 *  // Standard libraries.
 *  #include <memory>
 *  #include <string>
 *
 *  // C standard libraries.
 *  #include <cstdio>
 *  #include <cstdlib>
 *
 *  namespace SDL2 {
 *
 *  // A deleter is suitable if there is a handle to manage.
 *  // In the case of a "system" of this form,
 *  // it is simpler to simply use a destructor.
 *  class system {
 *    ~system() { SDL_Quit(); }
 *  };
 *
 *  // Creating a deleter requires knowing the handle type
 *  // and the function necessary to release the managed resource.
 *  // Note that, unlike `unique_ptr`,
 *  // the template parameter is a pointer `T*` instead of `T`.
 *  using window_deleter = handle_deleter<
 *    SDL_Window*, SDL_DestroyWinow>;
 *
 *  // This deleter can then be given to `std::unique_ptr`,
 *  // to automatically manage the lifetime of the window.
 *  using window = std::unique_ptr<SDL_window*, window_deleter>;
 *
 *  } // namespace SDL2
 *
 *  int main() {
 *    SDL_Init(SDL_INIT_EVENTS);
 *    SDL2::system system;
 *
 *    // Use the handle as it normally would be used.
 *    // It has the same interface as `std::unique_ptr`.
 *    SDL2:window window{SDL_CreateWindow(
 *        "Example", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
 *        1024, 768, 0u)};
 *    if (window.get() == nullptr) {
 *      std::fprintf(stderr, "Unable to create a window.\n");
 *      return EXIT_FAILURE;
 *    }
 *
 *    SDL_Event event_to_process;
 *    while (SDL_WaitEvent(&event) == 1) {
 *      if (event.type == SDL_QUIT) {
 *        break;
 *      }
 *    }
 *    return EXIT_SUCCESS;
 *    // Here, `window` is destroyed, and the `SDL_Quit` is called.
 *  }
 *  ```
 *
 *  This provides automatic lifetime management of `SDL_window`,
 *  assuming the constructed window is given directly to `window`.
 */
template <typename handle_type, void (*destroy_function)(handle_type)>
using handle_deleter =
    unsafe_handle_deleter<handle_type, void, destroy_function>;

/** \brief Wrapper around `std::unique_ptr` for use with C API.
 *
 *  \tparam deleter_type
 *  It must declare  a `deleter_type::pointer` type,
 *  as required by `Deleter` in `std::unique_ptr<T, Deleter>`.
 *  The "pointer" type must be a `NullablePointer`
 *  and, to handle an empty handle case,
 *  the expression `deleter_type()(deleter_type::pointer(nullptr))`
 *  must be well-defined.
 *  \tparam pointer
 *  The type to be stored to manage the underlying handle.
 *  This need not be the same as `handle_type`,
 *  but must be immplicitly convertable `handle_type`.
 *
 *  The main purpose of this class is to reduce the chance
 *  of forgetting to release resources.
 *  A "acquisition" part of RAII is assumed to be done by the caller,
 *  with the resulting handle being given immediately to this class,
 *  with no throwing code in between.
 *
 *  This is done by subclassing `std::unique_ptr`,
 *  so it has the same interface,  but with two changes:
 *
 *    * Since C API frequently accepts the underlying handle,
 *      using `unique_ptr.get()` to retrieve the stored handle
 *      becomes bothersome after a while.
 *      This class provides an implicit conversion
 *      to the `handle_type`.
 *      This possibly-unsafe nature is reflected in its name
 *      mimicking `auto_ptr`.
 *    * If `std::unique_ptr<T, Deleter>`
 *      takes a `Deleter` template parameter
 *      such that a nullable `Deleter::pointer` is defined,
 *      then the `T` template parameter is not used,
 *      but it must still be provided.
 *      This class removes the need to provide `T`.
 *
 *  The following is an example of how the class is intended to be used:

 *  ```cpp
 *  // Standard libraries.
 *  #include <memory>
 *  #include <string>
 *  #include <vector>
 *
 *  // C standard libraries.
 *  #include <cstdio>
 *  #include <cstdlib>
 *
 *  // Give a `deleter_type` to `auto_handle` to create a "handle" class.
 *  using file_deleter = handle_deleter<FILE*, std::fclose);
 *  using file = auto_handle<file_deleter>;
 *
 *  int main() {
 *    // Pass a created handle to the class immediately.
 *    std::string hello_filename{"README.md"};
 *    file hello_file{std::fopen(hello_filename.c_str(), "w+b")};
 *    // The `deleter_type` must be able to handle the "null" case,
 *    // so that after this error handling,
 *    // the program will not crash before exiting.
 *    if (hello_file.get() == nullptr) {
 *      std::fprintf(
 *        stderr, "Unable to open file: %s\n", hello_filename.c_str());
 *      return EXIT_FAILURE;
 *    }
 *
 *    // Do things with the handle.
 *    std::string source_buffer{"Hello, world!"};
 *    // There is implicit conversion to the underlying handle type.
 *    auto result = std::fwrite(
 *      source_buffer.data(),
 *      sizeof(decltype(source_buffer)::value_type),
 *      source_buffer.size(), hello_file);
 *    if (hello_file.get() == nullptr) {
 *      std::fprintf(
 *        stderr, "Unable to write to file: %s\n",
 *        hello_filename.c_str());
 *      return EXIT_FAILURE;
 *    }
 *
 *    // Here, `fclose(hello_file.get());` is automatically called,
 *    // with return value discarded.
 *  }
 *  ```

 */
template <
    typename deleter_type,
    typename pointer = typename deleter_type::pointer,
    typename parent_type = std::unique_ptr<pointer, deleter_type>>
class auto_handle : public parent_type {
public:
  using parent_type::parent_type;

  /** \brief The type to convert to in order to use C APIs. */
  using handle_type = typename deleter_type::handle_type;

  /** \brief Implicit conversion to `handle_type`.
   *
   *  This makes it easier to use in C API,
   *  by allowing the `auto_handle` to be used
   *  in place of where instances of `handle_type` is expected.
   *
   *  Note that this conversion may not work if the function is a macro,
   *  since the compiler may not be able to deduce
   *  what the type `auto_handle` should convert to.
   *  This is common, for example, in ncurses API.
   */
  operator handle_type() const { return parent_type::get(); }
};

/** \brief Callable releasing `FILE*` for use with `std::unique_ptr`.
 *
 *  This is mostly an implementation detail for \ref file.
 */
using file_deleter = unsafe_handle_deleter<FILE*, int, std::fclose>;

/** \brief RAII wrapper releasing `FILE*` when lifetime ends.
 *
 *  See \ref auto_handle for an exmaple of how to use this class.
 */
using file = auto_handle<file_deleter>;

} // namespace boni
