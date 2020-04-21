set(CMAKE_CXX_STANDARD 17)

# Vulkan
set(ENV{VULKAN_SDK} external/vulkansdk-macos/macOS)
find_package(Vulkan REQUIRED)

# environment path for u mac users
set(share_path ${CMAKE_CURRENT_SOURCE_DIR}/$ENV{VULKAN_SDK}/share/vulkan)
set(ENV{VK_ICD_FILENAMES} ${share_path}/icd.d/MoltenVK_icd.json)
set(ENV{VK_LAYER_PATH} ${share_path}/explicit_layer.d)

message("environment path for u mac users:
VK_ICD_FILENAMES=$ENV{VK_ICD_FILENAMES};VK_LAYER_PATH=$ENV{VK_LAYER_PATH}
or just run setup-env.sh")

#
# glfw
#
option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
option(GLFW_INSTALL "Generate installation target" OFF)
add_subdirectory(external/glfw)

# GLM
set(GLM_TEST_ENABLE OFF CACHE BOOL "" FORCE)
#add_subdirectory(external/glm)
FIND_PATH(GLM_INCLUDE_DIR "glm/glm.hpp"
        PATHS external/glm)
message(STATUS "GLM included at ${GLM_INCLUDE_DIR}")

include_directories(
        ${GLM_INCLUDE_DIR}
        include/
        external/
)

# i need all the libs? no just shaderc_combined
file(GLOB_RECURSE leebs
        $ENV{VULKAN_SDK}/lib/*.a
        )

find_library(shaderc_combined_LIB shaderc_combined $ENV{VULKAN_SDK}/lib)

#[[
add_library(shaderc_combined UNKNOWN IMPORTED)
set_target_properties(shaderc_combined PROPERTIES
        IMPORTED_LOCATION "${shaderc_combined_LIB}")

link_directories($ENV{VULKAN_SDK}/lib)
]]

link_libraries(Vulkan::Vulkan glfw ${shaderc_combined_LIB}
        ${leebs}
        #        shaderc_combined
        )

# Lab_X interface
#set(Lab_X "Lab 1")
#set(Lab_X "Lab 2")
set(Lab_X "Lab 3")
#set(Lab_X SimulatedLib)
file(GLOB_RECURSE Lab_X_files
        ${Lab_X}/main.cpp
        include/LHVulkan.*
        include/cube_data.h
        include/tiny_obj_loader.*
        )
add_executable(Lab_X ${Lab_X_files})
# do tha copying
add_custom_command(TARGET Lab_X
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/${Lab_X}/shaders $<TARGET_FILE_DIR:Lab_X>/shaders)

#[[
# Lab_1
set(Lab_1 "Lab 1")
file(GLOB_RECURSE Lab_1_files
        ${Lab_1}/main.cpp
        include/cube_data.h
        include/LHVulkan.*
        )
add_executable(Lab_1 ${Lab_1_files})
add_executable(Lab_1 ${Lab_1_files})
# do tha copying
add_custom_command(TARGET Lab_1
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/${Lab_1}/shaders $<TARGET_FILE_DIR:Lab_1>/shaders)

# Lab_2
set(Lab_2 "Lab 2")
file(GLOB_RECURSE Lab_2_files
        ${Lab_2}/main.cpp
        include/LHVulkan.*
        include/tiny_obj_loader.*
        include/cube_data.h
        )
add_executable(Lab_2 ${Lab_2_files})
# do tha copying
add_custom_command(TARGET Lab_2
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/${Lab_2}/shaders $<TARGET_FILE_DIR:Lab_2>/shaders)

# Lab_3
set(Lab_3 "Lab 3")
file(GLOB_RECURSE Lab_3_files
        ${Lab_3}/main.cpp
        include/tiny_obj_loader.*
        include/cube_data.h
        include/LHVulkan.*
        )
add_executable(Lab_3 ${Lab_3_files})
# do tha copying
add_custom_command(TARGET Lab_3
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/${Lab_3}/shaders $<TARGET_FILE_DIR:Lab_3>/shaders)

# Lab_4
set(Lab_4 "Lab 4")
file(GLOB_RECURSE Lab_4_files
        ${Lab_4}/main.cpp
        include/tiny_obj_loader.*
        include/cube_data.h
        include/LHVulkan.*
        )
add_executable(Lab_4 ${Lab_4_files})
# do tha copying
add_custom_command(TARGET Lab_4
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/${Lab_4}/shaders $<TARGET_FILE_DIR:Lab_4>/shaders)

# Lab_5
set(Lab_5 "Lab 5")
file(GLOB_RECURSE Lab_5_files
        ${Lab_5}/main.cpp
        include/tiny_obj_loader.*
        include/cube_data.h
        include/LHVulkan.*
        )
add_executable(Lab_5 ${Lab_5_files})
# do tha copying
add_custom_command(TARGET Lab_5
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/${Lab_5}/shaders $<TARGET_FILE_DIR:Lab_5>/shaders)

# Lab_6
set(Lab_6 "Lab 6")
file(GLOB_RECURSE Lab_6_files
        ${Lab_6}/main.cpp
        ${Lab_6}/texture.*
        include/tiny_obj_loader.*
        include/cube_data.h
        include/LHVulkan.*
        FreeImage.h
        )
add_executable(Lab_6 ${Lab_6_files})
target_include_directories(Lab_6 PUBLIC ${Lab_6})
target_link_libraries(Lab_6 lib/FreeImage.a)
# do tha copyings
add_custom_command(TARGET Lab_6
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/${Lab_6}/shaders $<TARGET_FILE_DIR:Lab_6>/shaders)
add_custom_command(TARGET Lab_6
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory ${Lab_6}/crate1.jpg ${Lab_6}/crate2.jpg $<TARGET_FILE_DIR:Lab_6>)

# Lab_7
set(Lab_7 "Lab 7")
file(GLOB_RECURSE Lab_7_files
        ${Lab_7}/main.cpp
        include/tiny_obj_loader.*
        include/cube_data.h
        include/LHVulkan.*
        )
add_executable(Lab_7 ${Lab_7_files})
add_custom_command(TARGET Lab_7
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/${Lab_7}/shaders $<TARGET_FILE_DIR:Lab_7>/shaders)

# Lab_10
set(Lab_10 "Lab 10")
file(GLOB_RECURSE Lab_10_files
        ${Lab_10}/main.cpp
        include/tiny_obj_loader.*
        include/cube_data.h
        include/LHVulkan.*
        )
add_executable(Lab_10 ${Lab_10_files})
add_custom_command(TARGET Lab_10
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/${Lab_10}/shaders $<TARGET_FILE_DIR:Lab_10>/shaders)

# SimulatedLib
set(SimulatedLib SimulatedLib)
file(GLOB_RECURSE SimulatedLib_files
        ${SimulatedLib}/main.cpp
        include/cube_data.h
        include/LHVulkan.*
        )
add_executable(SimulatedLib ${SimulatedLib_files})
add_custom_command(TARGET SimulatedLib
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/${SimulatedLib}/shaders $<TARGET_FILE_DIR:SimulatedLib>/shaders)
]]

#[[
# from https://github.com/stanthesoupking/Shade/blob/master/CMakeLists.txt
# Get GLFW3
find_package(glfw3)

# Check if library was found
if(NOT glfw3_FOUND)
	message("Failed to find local copy of GLFW, downloading copy of Github...")
	set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
	set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
	set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

	FetchContent_Declare(
		glfw3
		GIT_REPOSITORY https://github.com/glfw/glfw
	)

	FetchContent_MakeAvailable(glfw3)
endif()

# Get GLM
FetchContent_Declare(
	glm
	GIT_REPOSITORY https://github.com/g-truc/glm
)

set(GLM_TEST_ENABLE OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glm)
]]

#[[
# from https://cliutils.gitlab.io/modern-cmake/chapters/projects/submodule.html
execute_process(COMMAND ${GIT_EXECUTABLE} submodule add https://github.com/glfw/glfw external/glfw
        ${GIT_EXECUTABLE} submodule add https://github.com/g-truc/glm external/glm
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        RESULT_VARIABLE GIT_SUBMOD_RESULT)
]]

#[[
# from https://cmake.org/cmake/help/latest/module/ExternalProject.html#examples
include(ExternalProject)
ExternalProject_Add(foobar
  GIT_REPOSITORY    https://github.com/glfw/glfw external/glfw
)
]]
