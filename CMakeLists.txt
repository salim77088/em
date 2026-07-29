cmake_minimum_required(VERSION 3.16)
project(NEXUSEngine VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(MSVC)
    add_compile_options(/W3 /permissive- /Zc:__cplusplus /EHsc /bigobj /utf-8)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS _WINSOCK_DEPRECATED_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN)
else()
    add_compile_options(-Wall -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable)
    add_compile_options(-Wno-deprecated-declarations -Wno-sign-compare -Wno-format-truncation -Wno-stringop-truncation)
    if(APPLE)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "" FORCE)
    endif()
endif()

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# nlohmann/json
FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(json)

# glm
FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(glm)

# EnTT
FetchContent_Declare(entt
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG v3.13.2
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(entt)

# stb
FetchContent_Declare(stb
    URL https://github.com/nothings/stb/archive/refs/heads/master.tar.gz)
FetchContent_GetProperties(stb)
if(NOT stb_POPULATED)
    FetchContent_Populate(stb)
    add_library(stb INTERFACE)
    target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})
endif()

# miniaudio
FetchContent_Declare(miniaudio
    URL https://github.com/mackron/miniaudio/archive/refs/heads/master.tar.gz)
FetchContent_GetProperties(miniaudio)
if(NOT miniaudio_POPULATED)
    FetchContent_Populate(miniaudio)
    add_library(miniaudio INTERFACE)
    target_include_directories(miniaudio INTERFACE ${miniaudio_SOURCE_DIR})
endif()

# Dear ImGui
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.5
    GIT_SHALLOW TRUE)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
    set(IMGUI_SOURCES
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
    )
    add_library(imgui STATIC ${IMGUI_SOURCES})
    target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})
    target_compile_features(imgui PUBLIC cxx_std_17)
endif()

# SDL2 — use shared lib on Windows (static has issues), static on Linux/macOS
if(WIN32)
    set(SDL_SHARED ON CACHE BOOL "" FORCE)
    set(SDL_STATIC OFF CACHE BOOL "" FORCE)
else()
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
endif()
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.30.6
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(SDL2)
if(TARGET SDL2::SDL2)
    set(SDL2_LIBRARIES SDL2::SDL2)
elseif(TARGET SDL2-static)
    set(SDL2_LIBRARIES SDL2-static)
elseif(TARGET SDL2::SDL2-static)
    set(SDL2_LIBRARIES SDL2::SDL2-static)
else()
    set(SDL2_LIBRARIES SDL2)
endif()

# bgfx via bgfx.cmake — let it fetch its own bx/bimg/bgfx at known compatible revisions
set(BGFX_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BGFX_USE_DEBUG_SUFFIX OFF CACHE BOOL "" FORCE)
set(BGFX_CONFIG_RENDERER_VULKAN ON CACHE BOOL "" FORCE)
set(BGFX_CONFIG_RENDERER_METAL ON CACHE BOOL "" FORCE)
set(BGFX_CONFIG_RENDERER_DIRECT3D11 ON CACHE BOOL "" FORCE)
set(BGFX_CONFIG_RENDERER_OPENGL ON CACHE BOOL "" FORCE)
set(BGFX_CONFIG_USE_TINYST OFF CACHE BOOL "" FORCE)
FetchContent_Declare(bgfx.cmake
    GIT_REPOSITORY https://github.com/bkaradzic/bgfx.cmake.git
    GIT_TAG master
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(bgfx.cmake)

# Explicit include dirs in case bgfx.cmake doesn't propagate them correctly
if(TARGET bgfx)
    get_target_property(_bgfx_inc bgfx INTERFACE_INCLUDE_DIRECTORIES)
    message(STATUS "bgfx INTERFACE_INCLUDE_DIRECTORIES: ${_bgfx_inc}")
endif()

# Engine library
file(GLOB_RECURSE ENGINE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)
list(REMOVE_ITEM ENGINE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

add_library(nexus_engine STATIC ${ENGINE_SOURCES})
target_include_directories(nexus_engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    # Explicit bgfx include paths (bgfx.cmake uses git submodules in bgfx.cmake-src/)
    ${CMAKE_BINARY_DIR}/_deps/bgfx.cmake-src/bgfx/include
    ${CMAKE_BINARY_DIR}/_deps/bgfx.cmake-src/bx/include
    ${CMAKE_BINARY_DIR}/_deps/bgfx.cmake-src/bimg/include
    ${CMAKE_BINARY_DIR}/_deps/bgfx.cmake-src/bx/include/compat
    ${CMAKE_BINARY_DIR}/_deps/bgfx.cmake-src/bgfx/3rdparty
    ${CMAKE_BINARY_DIR}/_deps/bgfx.cmake-src/bimg/3rdparty
)
target_compile_definitions(nexus_engine PUBLIC GLM_ENABLE_EXPERIMENTAL)

target_link_libraries(nexus_engine PUBLIC
    bgfx
    bimg
    bx
    ${SDL2_LIBRARIES}
    imgui
    EnTT::EnTT
    glm::glm
    nlohmann_json::nlohmann_json
    stb
    miniaudio
)

add_executable(nexus src/main.cpp)
target_link_libraries(nexus PRIVATE nexus_engine)

if(APPLE)
    target_link_libraries(nexus PRIVATE "-framework Cocoa" "-framework QuartzCore" "-framework Metal" "-framework IOKit" "-framework ForceFeedback")
endif()
if(WIN32)
    target_link_libraries(nexus PRIVATE winmm imm32 version setupapi dinput8 dxguid ole32 oleaut32)
endif()

install(TARGETS nexus RUNTIME DESTINATION bin)
