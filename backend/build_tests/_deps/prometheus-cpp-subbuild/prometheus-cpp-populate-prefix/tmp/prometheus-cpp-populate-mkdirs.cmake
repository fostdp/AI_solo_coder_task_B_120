# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "D:/SOLO-2/AI_solo_coder_task_B_120/backend/build_tests/_deps/prometheus-cpp-src")
  file(MAKE_DIRECTORY "D:/SOLO-2/AI_solo_coder_task_B_120/backend/build_tests/_deps/prometheus-cpp-src")
endif()
file(MAKE_DIRECTORY
  "D:/SOLO-2/AI_solo_coder_task_B_120/backend/build_tests/_deps/prometheus-cpp-build"
  "D:/SOLO-2/AI_solo_coder_task_B_120/backend/build_tests/_deps/prometheus-cpp-subbuild/prometheus-cpp-populate-prefix"
  "D:/SOLO-2/AI_solo_coder_task_B_120/backend/build_tests/_deps/prometheus-cpp-subbuild/prometheus-cpp-populate-prefix/tmp"
  "D:/SOLO-2/AI_solo_coder_task_B_120/backend/build_tests/_deps/prometheus-cpp-subbuild/prometheus-cpp-populate-prefix/src/prometheus-cpp-populate-stamp"
  "D:/SOLO-2/AI_solo_coder_task_B_120/backend/build_tests/_deps/prometheus-cpp-subbuild/prometheus-cpp-populate-prefix/src"
  "D:/SOLO-2/AI_solo_coder_task_B_120/backend/build_tests/_deps/prometheus-cpp-subbuild/prometheus-cpp-populate-prefix/src/prometheus-cpp-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "D:/SOLO-2/AI_solo_coder_task_B_120/backend/build_tests/_deps/prometheus-cpp-subbuild/prometheus-cpp-populate-prefix/src/prometheus-cpp-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/SOLO-2/AI_solo_coder_task_B_120/backend/build_tests/_deps/prometheus-cpp-subbuild/prometheus-cpp-populate-prefix/src/prometheus-cpp-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
