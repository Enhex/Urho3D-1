// ImGui SDL2 binding with OpenGL (legacy, fixed pipeline)
// In this binding, ImTextureID is used to store an OpenGL 'GLuint' texture identifier. Read the FAQ about ImTextureID in imgui.cpp.
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)

// **DO NOT USE THIS CODE IF YOUR CODE/ENGINE IS USING MODERN OPENGL (SHADERS, VBO, VAO, etc.)**
// **Prefer using the code in the sdl_opengl3_example/ folder**
// See imgui_impl_sdl.cpp for details.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

struct SDL_Window;
typedef union SDL_Event SDL_Event;

bool        ImGui_ImplSdlGL2_Init(SDL_Window* window);
void        ImGui_ImplSdlGL2_Shutdown();
void        ImGui_ImplSdlGL2_NewFrame(SDL_Window* window);
bool        ImGui_ImplSdlGL2_ProcessEvent(SDL_Event* event);

// Use if you want to reset your rendering device without losing ImGui state.
void        ImGui_ImplSdlGL2_InvalidateDeviceObjects();
bool        ImGui_ImplSdlGL2_CreateDeviceObjects();
