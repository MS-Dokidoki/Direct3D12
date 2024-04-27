# *************************************************************
#	该文件为适用于 Direct3D 12-D3DApp 项目构建的 CMake 构建文件
# *************************************************************

# 最低 CMake 版本需求
cmake_minimum_required(VERSION 3.20)
project(D3DApp)

set(PROJECT_DEBUG_MODE 1)
# 内部变量配置
set(WINDOWS_SDK_PATH "D:\\Windows Kits\\10")
set(WINDOWS_SDK_VERSION "10.0.19041.0")
set(PROJECT_NAME "D3DApp")
set(PROJECT_VERSION "1.0.0")
set(PROJECT_ROOT_PATH ${CMAKE_SOURCE_DIR})
set(PROJECT_TOOLS_ROOT "C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX")
set(PROJECT_FRAME_ROOT "${PROJECT_TOOLS_ROOT}/D3DFrame")
set(PROJECT_D3D12XTK_ROOT "${PROJECT_TOOLS_ROOT}/DirectXTK")

set(WINDOWS_SDK_INCLUDE_ROOT_PATH "${WINDOWS_SDK_PATH}/include/${WINDOWS_SDK_VERSION}")
set(WINDOWS_SDK_LIBRARY_ROOT_PATH "${WINDOWS_SDK_PATH}/lib/${WINDOWS_SDK_VERSION}")

# 外部库
list(APPEND BUILD_ALL_LIBRARIES "${WINDOWS_SDK_LIBRARY_ROOT_PATH}/um/x64")
list(APPEND BUILD_ALL_LIBRARIES "${WINDOWS_SDK_LIBRARY_ROOT_PATH}/ucrt/x64")
list(APPEND BUILD_ALL_LIBRARIES "${WINDOWS_SDK_LIBRARY_ROOT_PATH}/um/x86")
list(APPEND BUILD_ALL_LIBRARIES "${WINDOWS_SDK_LIBRARY_ROOT_PATH}/ucrt/x86")
list(APPEND BUILD_ALL_LIBRARIES "${PROJECT_D3D12XTK_ROOT}/libs")

# 外部头文件
list(APPEND BUILD_ALL_INCLUDES "${WINDOWS_SDK_INCLUDE_ROOT_PATH}/um")
list(APPEND BUILD_ALL_INCLUDES "${WINDOWS_SDK_INCLUDE_ROOT_PATH}/ucrt")
list(APPEND BUILD_ALL_INCLUDES "${WINDOWS_SDK_INCLUDE_ROOT_PATH}/shared")
list(APPEND BUILD_ALL_INCLUDES "${WINDOWS_SDK_INCLUDE_ROOT_PATH}/winrt")
list(APPEND BUILD_ALL_INCLUDES "${WINDOWS_SDK_INCLUDE_ROOT_PATH}/cppwinrt")
list(APPEND BUILD_ALL_INCLUDES "${PROJECT_TOOLS_ROOT}")
list(APPEND BUILD_ALL_INCLUDES "${PROJECT_FRAME_ROOT}")
list(APPEND BUILD_ALL_INCLUDES "${PROJECT_D3D12XTK_ROOT}/includes")
# 内部库
file(GLOB BUILD_LIBS_SOURCES "${PROJECT_FRAME_ROOT}/*.cpp" "${PROJECT_FRAME_ROOT}/*.c")
list(REMOVE_ITEM BUILD_LIBS_SOURCES "${PROJECT_FRAME_ROOT}/test.cpp")
list(REMOVE_ITEM BUILD_LIBS_SOURCES "${PROJECT_FRAME_ROOT}/Main.cpp")

# 项目源文件
file(GLOB BUILD_ALL_SOURCES "${CMAKE_SOURCE_DIR}/sources/*.cpp")
set(BUILD_ALL_SOURCES_TEST ${BUILD_ALL_SOURCES})

list(REMOVE_ITEM BUILD_ALL_SOURCES "*test.cpp")

list(FIND BUILD_ALL_SOURCES_TEST "*test.cpp" pro_result)
if(${pro_result} EQUAL -1)
    set(PROJECT_DEBUG_MODE 0)
endif()
# *********************************************
# 处理源文件

# 打印基本信息
message(STATUS "--------------- BASE INFO -----------------")
message(STATUS "Windows SDK Path: ${WINDOWS_SDK_PATH}")
message(STATUS "Windows SDK Version: ${WINDOWS_SDK_VERSION}")
message(STATUS "Proejct Tools Path: ${PROJECT_TOOLS_ROOT}")
message(STATUS "Project Path: ${PROJECT_ROOT_PATH}")

message(STATUS "------------- PROJECT INFO ----------------")
message(STATUS "Project Name: ${PROJECT_NAME}")
message(STATUS "Project Version: ${PROJECT_VERSION}")
message(STATUS "Building source files...")

configure_file(config.h.in config.h)

# 添加头文件与链接文件
include_directories(${BUILD_ALL_INCLUDES} ${CMAKE_BINARY_DIR})
link_directories(${BUILD_ALL_LIBRARIES})

# 编译内部库
add_library(D3D12Frame STATIC ${BUILD_LIBS_SOURCES})

# 编译源文件
add_executable(D3DApp ${BUILD_ALL_SOURCES})
target_link_options(D3DApp PRIVATE "/Subsystem:WINDOWS")

if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
    target_link_libraries(D3DApp D3D12Frame DirectXTKx64.lib)
elseif()
    target_link_libraries(D3DApp D3D12Frame DirectXTK.lib)
endif()

if(${PROJECT_DEBUG_MODE} EQUAL 1)
    add_executable(D3DAppTest ${BUILD_ALL_SOURCES_TEST})
    if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
        target_link_libraries(D3DAppTest D3D12Frame DirectXTKx64.lib)
    elseif()
        target_link_libraries(D3DAppTest D3D12Frame DirectXTK.lib)
    endif()
endif()