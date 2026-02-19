cmake_minimum_required(VERSION 3.25)
project(vulkan-window)

set(CMAKE_CXX_STANDARD 20)

set(VULKAN_INCLUDE_DIR "$ENV{VULKAN_SDK}/Include")

add_compile_definitions(VULKAN_HPP_NO_STRUCT_CONSTRUCTORS)

include_directories(${VULKAN_INCLUDE_DIR})

find_program(SLANGC_EXECUTABLE slangc REQUIRED)

function (add_slang_shader_target TARGET)
  cmake_parse_arguments ("SHADER" "" "" "SOURCES" ${ARGN})
  set (SHADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders) # Better to use Binary Dir for build artifacts
  set (ENTRY_POINTS -entry vertMain -entry fragMain)
  
  add_custom_command (
          OUTPUT ${SHADERS_DIR}
          COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADERS_DIR}
  )
  
    add_custom_command (
        OUTPUT  ${SHADERS_DIR}/slang.spv
        # FIX: Ensure SHADER_SOURCES (plural from the parse_arguments) is used correctly
        COMMAND ${SLANGC_EXECUTABLE} ${SHADER_SOURCES} -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name -entry vertMain -entry fragMain -o slang.spv
        WORKING_DIRECTORY ${SHADERS_DIR}
        DEPENDS ${SHADER_SOURCES}
        COMMENT "Compiling Slang Shaders: ${SHADER_SOURCES}"
        VERBATIM
    )
  add_custom_target (${TARGET} DEPENDS ${SHADERS_DIR}/slang.spv)
endfunction()

find_package(glfw3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED) # Finds the SDK you just installed
find_package(glm CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)

# 1. Collect all header files in the current directory
file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")

# 2. Collect all source files (optional but recommended for consistency)
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

set(SHADER_SLANG_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.slang")

# 3. Create the executable using both lists
add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

target_precompile_headers(${PROJECT_NAME} PRIVATE pch.hpp)

# 4. Group the headers into the "Header Files" folder for IDEs (Visual Studio/Xcode)
source_group("Header Files" FILES ${HEADER_FILES})

# Damit der Compiler den Header mit <> findet (wenn er im Projektordner liegt)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME} PRIVATE 
    glfw 
    Vulkan::Vulkan
    glm::glm
    EnTT::EnTT
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