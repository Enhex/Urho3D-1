#
# Copyright (c) 2008-2017 the Urho3D project.
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

if (URHO3D_THIRDPARTY_SHARED_LIBS)
    set (THIRDPARTY_LIB_TYPE SHARED)
else ()
    set (THIRDPARTY_LIB_TYPE STATIC)
endif ()

add_subdirectory(FreeType)
add_subdirectory(LZ4)
add_subdirectory(PugiXml)
add_subdirectory(rapidjson)
add_subdirectory(SDL)
add_subdirectory(STB)
add_subdirectory(IconFontCppHeaders)
add_subdirectory(glm)
add_subdirectory(cr)
add_subdirectory(yaml-cpp)
if (DESKTOP AND NOT APPLE)
    add_subdirectory(crunch)
endif ()

if (URHO3D_SYSTEMUI)
    add_subdirectory(imgui)
endif ()

if (URHO3D_NETWORK)
    add_subdirectory (Civetweb)
    add_subdirectory (kNet)
endif ()

if (URHO3D_IK)
    add_subdirectory (ik)
endif ()

if (URHO3D_NAVIGATION)
    add_subdirectory (Detour)
    add_subdirectory (DetourCrowd)
    add_subdirectory (DetourTileCache)
    add_subdirectory (Recast)
endif ()

if (URHO3D_URHO2D)
    add_subdirectory (Box2D)
endif ()

if (URHO3D_WEBP)
    add_subdirectory (WebP)
endif ()

if (URHO3D_PHYSICS)
    add_subdirectory (newton-dynamics)
    add_subdirectory(StanHull)
endif ()

if (URHO3D_TOOLS)
    add_subdirectory(nativefiledialog)
endif ()

if (NOT ANDROID AND NOT ARM AND NOT WEB)
    if (URHO3D_OPENGL)
        add_subdirectory (GLEW)
    elseif (URHO3D_D3D9)
        add_subdirectory (MojoShader)
    endif ()
    if (WIN32 OR APPLE)
        add_subdirectory (LibCpuId)
    endif ()
endif ()

if (URHO3D_PROFILING)
    if (NOT TARGET GLEW AND URHO3D_TOOLS)
        add_subdirectory (GLEW)
    endif ()
    add_subdirectory (tracy)
endif ()

if (URHO3D_TOOLS)
    add_subdirectory(Assimp)
endif ()

if (URHO3D_TASKS)
    add_subdirectory(fcontext)
endif ()

add_subdirectory(CLI11)
add_subdirectory(fmt)
if (URHO3D_CSHARP)
    add_subdirectory(cppast)
    add_subdirectory(tinydir)
    add_subdirectory(spdlog)
endif ()

vs_group_subdirectory_targets(${CMAKE_CURRENT_SOURCE_DIR} ThirdParty)
