cmake_minimum_required(VERSION 3.25)
project(vulkan-window)

set(CMAKE_CXX_STANDARD 20)

set(VULKAN_INCLUDE_DIR "$ENV{VULKAN_SDK}/Include")

add_compile_definitions(VULKAN_HPP_NO_STRUCT_CONSTRUCTORS)

include_directories(${VULKAN_INCLUDE_DIR})

find_program(SLANGC_EXECUTABLE slangc REQUIRED)

function(add_slang_shader_target TARGET)
    cmake_parse_arguments("SHADER" "" "" "SOURCES" ${ARGN})
    set(SHADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)
  
    # Create the output directory once
    add_custom_command(
        OUTPUT ${SHADERS_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADERS_DIR}
    )
  
    set(SPV_OUTPUTS "")
    
    # Loop over every .slang file provided
    foreach(SLANG_SRC IN LISTS SHADER_SOURCES)
        # Extract the base name (e.g., "skybox" from "skybox.slang")
        get_filename_component(FILE_NAME ${SLANG_SRC} NAME_WE)
        set(SPV_OUT ${SHADERS_DIR}/${FILE_NAME}.spv)
        
        add_custom_command(
            OUTPUT ${SPV_OUT}
            COMMAND ${SLANGC_EXECUTABLE} ${SLANG_SRC} -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name -entry vertMain -entry fragMain -o ${SPV_OUT}
            DEPENDS ${SLANG_SRC} ${SHADERS_DIR}
            COMMENT "Compiling Slang Shader: ${FILE_NAME}.slang -> ${FILE_NAME}.spv"
            VERBATIM
        )
        
        list(APPEND SPV_OUTPUTS ${SPV_OUT})
    endforeach()
  
    # The target depends on all generated .spv files
    add_custom_target(${TARGET} ALL DEPENDS ${SPV_OUTPUTS})
endfunction()

find_package(glfw3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(imguizmo CONFIG REQUIRED)

file(GLOB_RECURSE ALL_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
)

# Automatically find ALL .slang files in the shaders directory
file(GLOB SHADER_SLANG_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.slang")

add_executable(${PROJECT_NAME} ${ALL_SOURCES})

target_precompile_headers(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/pch.hpp")

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "Source Files" FILES ${ALL_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src" 
    "${Stb_INCLUDE_DIR}"
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    glfw 
    Vulkan::Vulkan
    glm::glm
    EnTT::EnTT
    imgui::imgui
    imguizmo::imguizmo
)

# Pass all found .slang files to our compilation function
add_slang_shader_target(compiled_shaders SOURCES ${SHADER_SLANG_SOURCES})

add_dependencies(${PROJECT_NAME} compiled_shaders)

# Copy the entire 'shaders' output directory to the executable directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory 
    "${CMAKE_CURRENT_BINARY_DIR}/shaders" "${CMAKE_CURRENT_BINARY_DIR}/shaders"
    COMMENT "Copying compiled shaders to executable directory..."
)

# Kopiert den Ordner 'resources' in das Ausführungsverzeichnis der Exe
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/resources 
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})