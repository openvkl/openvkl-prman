## Copyright 2022 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    option(ENABLE_SUBMODULE_PLUGINS "Enable cloning of plugins provided via Git submodules" OFF)
    if(ENABLE_SUBMODULE_PLUGINS)
        message(STATUS "Submodule update")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init --recursive failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
endif()

# add all plugins
file(GLOB plugins RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/ *)
foreach(plugin ${plugins})
  if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${plugin}/CMakeLists.txt)
    add_subdirectory(${plugin})
  endif (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${plugin}/CMakeLists.txt)
endforeach(plugin ${plugins})
