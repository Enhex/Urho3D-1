#
# Copyright (c) 2008-2018 the Urho3D project.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

project(CSHARP)

set (CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(Generator)

set (CSHARP_BINDING_GENERATOR_OPTIONS ${CMAKE_BINARY_DIR}/CSharpBindings/generator_options.txt CACHE INTERNAL "")
set (CSHARP_MODULE_INDEX 0 CACHE INTERNAL "")
file(WRITE ${CSHARP_BINDING_GENERATOR_OPTIONS} "")

macro (__TARGET_GET_PROPERTIES_RECURSIVE OUTPUT TARGET PROPERTY)
    get_target_property(values ${TARGET} ${PROPERTY})
    if (values)
        list (APPEND ${OUTPUT} ${values})
    endif ()
    get_target_property(values ${TARGET} INTERFACE_LINK_LIBRARIES)
    if (values)
        foreach(lib ${values})
            if (TARGET ${lib})
                __TARGET_GET_PROPERTIES_RECURSIVE(${OUTPUT} ${lib} ${PROPERTY})
            endif ()
        endforeach()
    endif()
endmacro()

macro (add_target_csharp TARGET PROJECT_FILE)
    if (WIN32 AND NOT URHO3D_WITH_MONO)
        include_external_msproject(${TARGET} ${PROJECT_FILE} TYPE FAE04EC0-301F-11D3-BF4B-00C04F79EFBC CSharpBindings)
    else ()
        if (WIN32)
            find_package(Mono REQUIRED)
            find_program(MSBUILD xbuild.bat PATHS ${MONO_PATH}/bin)
        elseif (APPLE)
            find_program(MSBUILD xbuild)
        else ()
            find_program(MSBUILD msbuild)
        endif ()
        if (NOT MSBUILD)
            message(FATAL_ERROR "MSBuild could not be found.")
        endif ()
        if ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Linux")
            # Workaround for some cases where csc has issues when invoked by CMake.
            set (TERM_WORKAROUND env TERM=xterm)
        endif ()
        if (${CMAKE_BUILD_TYPE})
            set (CSHARP_BUILD_TYPE ${CMAKE_BUILD_TYPE})
        else ()
            set (CSHARP_BUILD_TYPE $<CONFIG>)
        endif ()

        if (CMAKE_SIZEOF_VOID_P EQUAL 8)
            set (CSHARP_PLATFORM x64)
        else ()
            set (CSHARP_PLATFORM x86)
        endif ()

        add_custom_target(${TARGET} COMMAND ${TERM_WORKAROUND} ${MSBUILD} ${PROJECT_FILE} ${ARGN}
            /p:BuildDir="${CMAKE_BINARY_DIR}"
            /p:Configuration=${CSHARP_BUILD_TYPE} /p:Platform=${CSHARP_PLATFORM} /consoleloggerparameters:ErrorsOnly)
        set_target_properties(${TARGET} PROPERTIES EXCLUDE_FROM_ALL OFF)
    endif ()
endmacro ()

macro (csharp_bind_target)
    if (NOT URHO3D_CSHARP)
        return ()
    endif ()

    cmake_parse_arguments(BIND "INTERNAL;EXTERNAL" "TARGET;MANAGED_TARGET" "" ${ARGN})

    get_target_property(BIND_SOURCE_DIR ${BIND_TARGET} SOURCE_DIR)

    if (NOT INTERNAL AND NOT EXTERNAL)
        if (BUILD_SHARED_LIBS)
            set (BIND_INTERNAL ON)
        else ()
            set (BIND_EXTERNAL ON)
        endif ()
    endif ()

    if (NOT BIND_MANAGED_TARGET)
        set (BIND_MANAGED_TARGET ${BIND_TARGET}Net)
    endif ()

    set (BIND_CSHARP_DIR ${BIND_SOURCE_DIR}/CSharp)
    if (NOT IS_DIRECTORY ${BIND_CSHARP_DIR})
        unset (BIND_CSHARP_DIR)
    endif ()

    if (BIND_CSHARP_DIR)
        set (RULES ${BIND_CSHARP_DIR}/config.json)
    endif ()

    if (NOT EXISTS ${RULES})
        message(FATAL_ERROR "config.json was not found. It must reside in BIND_SOURCE_DIR or BIND_CSHARP_DIR.")
    endif ()

    # Parse bindings using same compile definitions as built target
    __TARGET_GET_PROPERTIES_RECURSIVE(INCLUDES ${BIND_TARGET} INTERFACE_INCLUDE_DIRECTORIES)
    __TARGET_GET_PROPERTIES_RECURSIVE(DEFINES  ${BIND_TARGET} INTERFACE_COMPILE_DEFINITIONS)
    __TARGET_GET_PROPERTIES_RECURSIVE(OPTIONS  ${BIND_TARGET} INTERFACE_COMPILE_OPTIONS)
    if (INCLUDES)
        list (REMOVE_DUPLICATES INCLUDES)
    endif ()
    if (DEFINES)
        list (REMOVE_DUPLICATES DEFINES)
    endif ()
    if (OPTIONS)
        list (REMOVE_DUPLICATES OPTIONS)
    endif ()
    list(APPEND GENERATOR_OPTIONS bind${CSHARP_MODULE_INDEX})
    math(EXPR CSHARP_MODULE_INDEX "${CSHARP_MODULE_INDEX} + 1")
    set (CSHARP_MODULE_INDEX ${CSHARP_MODULE_INDEX} CACHE INTERNAL "")
    foreach(item ${INCLUDES})
        if ("${item}" MATCHES "\\$<INSTALL_INTERFACE:.+")
            continue()
        endif ()
        if ("${item}" MATCHES "\\$<BUILD_INTERFACE:.+")
            string(LENGTH "${item}" len)
            math(EXPR len "${len} - 19")
            string(SUBSTRING "${item}" 18 ${len} item)
        endif ()
        list(APPEND GENERATOR_OPTIONS -I${item})
    endforeach()
    foreach(item ${DEFINES})
        list(APPEND GENERATOR_OPTIONS -D${item})
    endforeach()
    foreach(item ${OPTIONS})
        list(APPEND GENERATOR_OPTIONS -O${item})
    endforeach()

    get_target_property(_TARGET_TYPE ${BIND_TARGET} TYPE)
    if(_TARGET_TYPE STREQUAL "STATIC_LIBRARY")
        list (APPEND GENERATOR_OPTIONS --static)
    endif ()
    if (SNK_PUB_KEY)
        list (APPEND GENERATOR_OPTIONS --signed-with=${SNK_PUB_KEY})
    endif ()

    if (BIND_INTERNAL)
        set (CSHARP_LIBRARY_NAME ${BIND_TARGET})
    elseif (BIND_EXTERNAL)
        set (CSHARP_LIBRARY_NAME ${BIND_TARGET}CSharp)
    else ()
        message(FATAL_ERROR "Please pass INTERNAL or EXTERNAL to csharp_bind_target()")
    endif ()

    set (BINDINGS_OUTPUT_DIR ${CMAKE_BINARY_DIR}/CSharpBindings/${BIND_TARGET})
    set (COMMON_DIR ${CSHARP_SOURCE_DIR}/Common)

    # Generated source files
    file(MAKE_DIRECTORY ${BINDINGS_OUTPUT_DIR}/Native)
    file(MAKE_DIRECTORY ${BINDINGS_OUTPUT_DIR}/CSharp)
    set (CSHARP_SOURCE_FILES_GENERATED
        ${BINDINGS_OUTPUT_DIR}/Native/${BIND_TARGET}CApi.cpp
        ${BINDINGS_OUTPUT_DIR}/Native/${BIND_TARGET}ClassWrappers.hpp
        ${BINDINGS_OUTPUT_DIR}/Native/${BIND_TARGET}RegisterFactories.cpp
    )
    set (COMMON_NET_SOURCE_FILES_GENERATED
        ${BINDINGS_OUTPUT_DIR}/CSharp/${BIND_TARGET}PInvoke.cs
        ${BINDINGS_OUTPUT_DIR}/CSharp/${BIND_TARGET}CSharp.cs
        ${BINDINGS_OUTPUT_DIR}/CSharp/Config.cs
    )
    set_source_files_properties(${CSHARP_SOURCE_FILES_GENERATED} ${COMMON_NET_SOURCE_FILES_GENERATED} PROPERTIES GENERATED TRUE)

    # Ensure that all files of generated C# project exist before build starts. VS scans filesystem
    # on start and generated files will not be included in the build after they are generated.
    foreach (file ${COMMON_NET_SOURCE_FILES_GENERATED} ${CSHARP_SOURCE_FILES_GENERATED})
        if (NOT EXISTS ${file})
            execute_process (COMMAND ${CMAKE_COMMAND} -E touch ${file})
        endif ()
    endforeach ()

    # CSharp target (native)
    file (GLOB_RECURSE COMMON_CSHARP_SOURCE_FILES_MANUAL
        ${COMMON_DIR}/Native/*.cpp ${BIND_CSHARP_DIR}/Native/*.cpp)

    if (BIND_INTERNAL)
        target_sources(${BIND_TARGET} PRIVATE ${COMMON_CSHARP_SOURCE_FILES_MANUAL} ${CSHARP_SOURCE_FILES_GENERATED})
    elseif (BIND_EXTERNAL)
        add_library(${CSHARP_LIBRARY_NAME} SHARED ${COMMON_CSHARP_SOURCE_FILES_MANUAL} ${CSHARP_SOURCE_FILES_GENERATED})
        target_link_libraries(${CSHARP_LIBRARY_NAME} ${BIND_TARGET})
        install(TARGETS ${CSHARP_LIBRARY_NAME}
            LIBRARY DESTINATION ${DEST_LIBRARY_DIR}
            RUNTIME DESTINATION ${DEST_BIN_DIR}
            ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR}
        )
    endif ()

    # Finalize option list
    list (APPEND GENERATOR_OPTIONS ${CSHARP_LIBRARY_NAME} ${RULES} ${BIND_SOURCE_DIR} ${BINDINGS_OUTPUT_DIR})
    foreach (opt ${GENERATOR_OPTIONS})
        file(APPEND ${CSHARP_BINDING_GENERATOR_OPTIONS} "${opt}\n")
    endforeach ()

    add_dependencies(${BIND_TARGET} CSharpBindings)
    if (CLANG)
        target_compile_options(${CSHARP_LIBRARY_NAME} PRIVATE -Wno-return-type-c-linkage)
    endif ()
    target_include_directories(${CSHARP_LIBRARY_NAME} PRIVATE
        ${COMMON_DIR}/Native ${BIND_CSHARP_DIR}/Native ${BINDINGS_OUTPUT_DIR}/Native)

    if (URHO3D_WITH_MONO)
        find_package(Mono REQUIRED)
        target_include_directories(${CSHARP_LIBRARY_NAME} PRIVATE ${MONO_INCLUDE_DIRS})
        target_link_libraries(${CSHARP_LIBRARY_NAME} ${MONO_LIBRARIES})
        target_compile_options(${CSHARP_LIBRARY_NAME} PRIVATE ${MONO_CFLAGS})
    endif ()

    if (MSVC)
        set (NET_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>)
    else ()
        set (NET_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
        # Needed for mono on unixes but not on windows.
        set (FACADES Facades/)
    endif ()
    if (EXISTS "${BIND_CSHARP_DIR}/${BIND_MANAGED_TARGET}.csproj")
        add_target_csharp(${BIND_MANAGED_TARGET} ${BIND_CSHARP_DIR}/${BIND_MANAGED_TARGET}.csproj)
        add_dependencies(${BIND_MANAGED_TARGET} ${CSHARP_LIBRARY_NAME})
        install (FILES ${NET_OUTPUT_DIRECTORY}/${BIND_MANAGED_TARGET}.dll DESTINATION ${DEST_LIBRARY_DIR})
    endif ()
endmacro ()

# Target for manually refreshing binding code
add_custom_target(CSharpBindings
    COMMAND $<TARGET_FILE:CSharpGenerator> ${CSHARP_BINDING_GENERATOR_OPTIONS}
    DEPENDS CSharpGenerator
    COMMENT "Generating CSharp bindings")

# Dependencies for bindings
file (DOWNLOAD https://www.nuget.org/api/v2/package/System.Buffers/4.4.0 ${CMAKE_BINARY_DIR}/System.Buffers/pkg.zip)
execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar "xf" "pkg.zip" --format=zip
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/System.Buffers
)

if (WIN32)
    add_custom_command(TARGET CSharpBindings POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/System.Buffers/ref/netstandard1.1/System.Buffers.dll ${CMAKE_BINARY_DIR}/${DEST_LIBRARY_DIR_CONFIG}/)
    if (URHO3D_WITH_MONO)
        add_custom_command(TARGET CSharpBindings POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${MONO_PATH}/lib/mono/4.5/Facades/System.Runtime.dll ${CMAKE_BINARY_DIR}/${DEST_LIBRARY_DIR_CONFIG}/)
        install (FILES
            ${CMAKE_BINARY_DIR}/${DEST_LIBRARY_DIR_CONFIG}/System.Runtime.dll
            DESTINATION ${DEST_LIBRARY_DIR}
        )
    endif ()
    install (FILES
        ${CMAKE_BINARY_DIR}/${DEST_LIBRARY_DIR_CONFIG}/System.Buffers.dll
        DESTINATION ${DEST_LIBRARY_DIR}
    )
endif ()

find_program(SN sn)
if (NOT EXISTS ${CMAKE_BINARY_DIR}/CSharp.snk)
    execute_process(COMMAND ${SN} -k ${CMAKE_BINARY_DIR}/CSharp.snk)
endif ()
if (NOT EXISTS ${CMAKE_BINARY_DIR}/CSharp.snk.pub)
    execute_process(COMMAND ${SN} -p ${CMAKE_BINARY_DIR}/CSharp.snk ${CMAKE_BINARY_DIR}/CSharp.snk.pub)
endif ()

execute_process(
    COMMAND ${SN} -tp ${CMAKE_BINARY_DIR}/CSharp.snk.pub
    OUTPUT_VARIABLE SNK_PUB_KEY
)
string(REGEX MATCH "Public [Kk]ey(.+)?:[0-9a-f\r\n]+\r?\n\r?\n" SNK_PUB_KEY "${SNK_PUB_KEY}")
string(REGEX REPLACE "Public [Kk]ey(.+)?:" "" SNK_PUB_KEY "${SNK_PUB_KEY}")
string(REGEX REPLACE "[ \r\n]+" "" SNK_PUB_KEY "${SNK_PUB_KEY}")
set(SNK_PUB_KEY "${SNK_PUB_KEY}" CACHE STRING "Public key for .NET assemblies" FORCE)
