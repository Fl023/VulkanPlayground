cmake_minimum_required(VERSION 3.25)
project(vulkan-window)

set(CMAKE_CXX_STANDARD 20)

set(VULKAN_INCLUDE_DIR "$ENV{VULKAN_SDK}/Include")

add_compile_definitions(VULKAN_HPP_NO_STRUCT_CONSTRUCTORS)

include_directories(${VULKAN_INCLUDE_DIR})

find_program(SLANGC_EXECUTABLE slangc REQUIRED)

function (add_slang_shader_target TARGET)
  cmake_parse_arguments ("SHADER" "" "" "SOURCES" ${ARGN})
  set (SHADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)
  set (ENTRY_POINTS -entry vertMain -entry fragMain)
  
  add_custom_command (
          OUTPUT ${SHADERS_DIR}
          COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADERS_DIR}
  )
  
    add_custom_command (
        OUTPUT  ${SHADERS_DIR}/slang.spv
        COMMAND ${SLANGC_EXECUTABLE} ${SHADER_SOURCES} -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name -entry vertMain -entry fragMain -o slang.spv
        WORKING_DIRECTORY ${SHADERS_DIR}
        DEPENDS ${SHADER_SOURCES}
        COMMENT "Compiling Slang Shaders: ${SHADER_SOURCES}"
        VERBATIM
    )
  add_custom_target (${TARGET} DEPENDS ${SHADERS_DIR}/slang.spv)
endfunction()

find_package(glfw3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(imgui CONFIG REQUIRED)

file(GLOB_RECURSE ALL_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
)

set(SHADER_SLANG_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.slang")

add_executable(${PROJECT_NAME} ${ALL_SOURCES})

target_precompile_headers(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/pch.hpp")

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "Source Files" FILES ${ALL_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src" # Ermöglicht #include "Renderer/VulkanDevice.hpp"
    "${Stb_INCLUDE_DIR}"
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    glfw 
    Vulkan::Vulkan
    glm::glm
    EnTT::EnTT
    imgui::imgui
)

# This creates a new build target called "compiled_shaders"
add_slang_shader_target(compiled_shaders SOURCES ${SHADER_SLANG_SOURCES})

# This makes sure the shaders are compiled BEFORE the executable is built
add_dependencies(${PROJECT_NAME} compiled_shaders)

# OPTIONAL: Copy the compiled slang.spv to your executable directory so your code can find it
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different 
    "${CMAKE_CURRENT_BINARY_DIR}/shaders/slang.spv" "${CMAKE_BINARY_DIR}/slang.spv"
)

# Kopiert den Ordner 'resources' in das Ausf�hrungsverzeichnis der Exe
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/resources 
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})