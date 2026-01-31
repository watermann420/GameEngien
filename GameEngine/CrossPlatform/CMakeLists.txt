cmake_minimum_required(VERSION 3.16)
project(GameEngineCross LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(GECROSS_ENABLE_VULKAN "Enable Vulkan renderer" ON)
option(GECROSS_ENABLE_SDL2 "Enable SDL2 windowing" ON)
option(GECROSS_ENABLE_OPENAL "Enable OpenAL audio" ON)

add_executable(GameEngineCross
    src/main.cpp
    src/TestScene.h
    src/TestScene.cpp
    src/Renderer2D.h
    src/Renderer2D.cpp
    src/BmpWriter.h
    src/BmpWriter.cpp
    src/VulkanRenderer.h
    src/VulkanRenderer.cpp
    src/AudioEngine.h
    src/AudioEngine.cpp
    ../../GameEngineCore/textur/VideoRenderPipline/TexturePool.cpp
    ../../GameEngineCore/textur/VideoRenderPipline/VideoTexture.cpp
    ../../GameEngineCore/textur/VideoRenderPipline/TextureCache.cpp
    ../scripts/run_smoke_tests.cpp
)

if (MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

target_include_directories(GameEngineCross PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/../GameEngineCore
    ${CMAKE_CURRENT_LIST_DIR}/../GameEngineCore/textur/VideoRenderPipline
)

if (GECROSS_ENABLE_VULKAN)
    find_package(Vulkan)
    if (Vulkan_FOUND)
        target_compile_definitions(GameEngineCross PRIVATE GECROSS_HAS_VULKAN=1)
        target_link_libraries(GameEngineCross PRIVATE Vulkan::Vulkan)
    else()
        message(WARNING "Vulkan not found, disabling Vulkan renderer")
    endif()
endif()

if (GECROSS_ENABLE_SDL2)
    find_package(SDL2)
    if (SDL2_FOUND)
        target_compile_definitions(GameEngineCross PRIVATE GECROSS_HAS_SDL2=1)
        target_link_libraries(GameEngineCross PRIVATE SDL2::SDL2)
    else()
        message(WARNING "SDL2 not found, disabling windowed renderer")
    endif()
endif()

if (GECROSS_ENABLE_OPENAL)
    find_package(OpenAL)
    if (OpenAL_FOUND)
        target_compile_definitions(GameEngineCross PRIVATE GECROSS_HAS_OPENAL=1)
        target_link_libraries(GameEngineCross PRIVATE OpenAL::OpenAL)
    else()
        message(WARNING "OpenAL not found, disabling audio playback")
    endif()
endif()
