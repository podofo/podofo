# Adapted from https://github.com/aminya/project_options

include_guard()

function(delete_inplace IN_FILE pattern)
  # create list of lines form the contens of a file
  file (STRINGS ${IN_FILE} LINES)

  # overwrite the file....
  file(WRITE ${IN_FILE} "")
  
  # loop through the lines,
  # remove unwanted parts 
  # and write the (changed) line ...
  foreach(LINE IN LISTS LINES)
    string(REGEX REPLACE ${pattern} "" STRIPPED "${LINE}")
    file(APPEND ${IN_FILE} "${STRIPPED}\n")
  endforeach()
endfunction()

function(conan_get_version conan_current_version)
  find_program(conan_command "conan" REQUIRED)
  execute_process(
    COMMAND ${conan_command} --version
    OUTPUT_VARIABLE conan_output
    RESULT_VARIABLE conan_result
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  if(conan_result)
    message(FATAL_ERROR "Error when trying to run Conan")
  endif()

  string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" conan_version ${conan_output})
  set(${conan_current_version} ${conan_version} PARENT_SCOPE)
endfunction()

# Run Conan 2 for dependency management
macro(_run_conan2)
  set(options)
  set(one_value_args)
  set(multi_value_args HOST_PROFILE BUILD_PROFILE INSTALL_ARGS)
  cmake_parse_arguments(_args "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  if(CMAKE_VERSION VERSION_LESS "3.24.0")
    message(FATAL_ERROR "Conan integration only supports cmake 3.24+, please update your cmake.")
  endif()

  conan_get_version(_conan_current_version)
  if(_conan_current_version VERSION_LESS "2.0.5")
    message(
      FATAL_ERROR "Conan integration only supports conan 2.0.5+, please update your conan.")
  endif()

  # Download automatically, you can also just copy the conan.cmake file
  if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan_provider.cmake")
    message(STATUS "Downloading conan_provider.cmake from https://github.com/conan-io/cmake-conan")
    file(
      DOWNLOAD
      "https://raw.githubusercontent.com/conan-io/cmake-conan/f6464d1e13ef7a47c569f5061f9607ea63339d39/conan_provider.cmake"
      "${CMAKE_BINARY_DIR}/conan_provider.cmake"
      EXPECTED_HASH SHA256=0a5eb4afbdd94faf06dcbf82d3244331605ef2176de32c09ea9376e768cbb0fc
      # TLS_VERIFY ON # fails on some systems
    )

    # This should be a git patch, but applying a patch won't work on Windows
    delete_inplace("${CMAKE_BINARY_DIR}/conan_provider.cmake" ".*cmaketoolchain\:generator.*")
  endif()

  if(NOT _args_HOST_PROFILE)
    set(_args_HOST_PROFILE "default;auto-cmake")
  endif()

  if(NOT _args_BUILD_PROFILE)
    set(_args_BUILD_PROFILE "default")
  endif()

  if(NOT _args_INSTALL_ARGS)
    set(_args_INSTALL_ARGS "--build=missing")
  endif()

  set(CONAN_HOST_PROFILE "${_args_HOST_PROFILE}" CACHE STRING "Conan host profile" FORCE)
  set(CONAN_BUILD_PROFILE "${_args_BUILD_PROFILE}" CACHE STRING "Conan build profile" FORCE)
  set(CONAN_INSTALL_ARGS "${_args_INSTALL_ARGS}" CACHE STRING "Command line arguments for conan install"
                                                       FORCE
  )

  # A workaround from https://github.com/conan-io/cmake-conan/issues/595
  list(APPEND CMAKE_PROJECT_TOP_LEVEL_INCLUDES "${CMAKE_BINARY_DIR}/conan_provider.cmake")

  # Add this to invoke conan even when there's no find_package in CMakeLists.txt.
  # This helps users get the third-party package names, which is used in later find_package.
  cmake_language(
    DEFER
    DIRECTORY
    "${CMAKE_CURRENT_SOURCE_DIR}"
    CALL
    find_package
    Git
    QUIET
  )
endmacro()

#[[.rst:

``run_conan``
=============

Install conan and conan dependencies:

.. code:: cmake

  run_conan()

.. code:: cmake

  run_conan(
    HOST_PROFILE default auto-cmake
    BUILD_PROFILE default
    INSTALL_ARGS --build=missing
  )

Note that it should be called before defining ``project()``.

Named String:

- Values are semicolon separated, e.g. ``"--build=never;--update;--lockfile-out=''"``.
  However, you can make use of the cmake behaviour that automatically concatenates
  multiple space separated string into a semicolon seperated list, e.g.
  ``--build=never --update --lockfile-out=''``.

-  ``HOST_PROFILE``: (Defaults to ``"default;auto-cmake"``). This option
  sets the host profile used by conan. When ``auto-cmake`` is specified,
  cmake-conan will invoke conan's autodetection mechanism which tries to
  guess the system defaults. If multiple profiles are specified, a
  `compound profile <https://docs.conan.io/2.0/reference/commands/install.html#profiles-settings-options-conf>`_
  will be used - compounded from left to right, where right has the highest priority.

-  ``BUILD_PROFILE``: (Defaults to ``"default"``). This option
  sets the build profile used by conan. If multiple profiles are specified,
  a `compound profile <https://docs.conan.io/2.0/reference/commands/install.html#profiles-settings-options-conf>`_
  will be used - compounded from left to right, where right has the highest priority.

-  ``INSTALL_ARGS``: (Defaults to ``"--build=missing"``). This option
  customizes ``conan install`` command invocation. Note that ``--build``
  must be specified, otherwise conan will revert to its default behaviour.

  - Two arguments are reserved to the dependency provider implementation
    and must not be set: the path to a ``conanfile.txt|.py``, and the output
    format (``--format``).

]]
macro(run_conan)
  conan_get_version(_conan_current_version)

  if(_conan_current_version VERSION_LESS "2.0.0")
    message(FATAL_ERROR "Conan integration only supports conan-2.")
  else()
    _run_conan2(${ARGN})
  endif()
endmacro()
