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

set (CMAKE_CXX_STANDARD 14)

file (GLOB_RECURSE COMMON_SOURCE_FILES common/*.cpp common/*.hpp common/*.h)
file (GLOB_RECURSE CLIENT_SOURCE_FILES client/*.cpp client/*.hpp client/*.h)
file (GLOB_RECURSE SERVER_SOURCE_FILES server/*.cpp server/*.hpp server/*.h)

set (TRACY_OPTIONS 
    PRIVATE -DNOMINMAX=1 -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE -DWIN32_LEAN_AND_MEAN -DNOMINMAX -D_USE_MATH_DEFINES 
    PUBLIC -DTRACY_ENABLE=1
)

add_library (tracy-lib STATIC ${CLIENT_SOURCE_FILES} ${COMMON_SOURCE_FILES})
target_link_libraries(tracy-lib LZ4)
target_compile_definitions(tracy-lib ${TRACY_OPTIONS})
if (WIN32)
    target_compile_definitions(tracy-lib PUBLIC -DNOMINMAX=1)
endif ()
target_include_directories(tracy-lib
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PRIVATE include/tracy
)

if (URHO3D_TOOLS AND URHO3D_SYSTEMUI)
    add_executable (tracy-profiler WIN32 ${SERVER_SOURCE_FILES} ${COMMON_SOURCE_FILES}
        standalone/src/main.cpp standalone/src/imgui_impl_sdl.cpp)
    target_link_libraries(tracy-profiler Urho3D nativefiledialog)
    target_compile_definitions(tracy-profiler PUBLIC -DTRACY_FILESELECTOR=1)
    set_target_properties(tracy-profiler PROPERTIES OUTPUT_NAME Profiler)
    if (APPLE)
        target_link_libraries(tracy-profiler pthread dl "-framework OpenGl")
    elseif (UNIX)
        target_link_libraries(tracy-profiler pthread dl GL)
    elseif (WIN32)
        target_link_libraries(tracy-profiler opengl32.lib ws2_32.lib winmm.lib version.lib)
        target_compile_definitions(tracy-profiler ${TRACY_OPTIONS})
        if (MSVC)
            target_compile_options(tracy-profiler PRIVATE /permissive-)
        endif ()
    endif ()
endif ()

install (TARGETS tracy-lib EXPORT Urho3D
    LIBRARY DESTINATION ${DEST_LIBRARY_DIR}
    RUNTIME DESTINATION ${DEST_BIN_DIR}
    ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR}
    )
install (DIRECTORY include/ DESTINATION ${DEST_THIRDPARTY_HEADERS_DIR}/ FILES_MATCHING PATTERN *.h)
