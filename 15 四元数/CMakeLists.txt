cmake_minimum_required(VERSION 3.0)
project(D3DApp)

# 环境变量
set(D3D12PROJECT_ROOT_PATH "C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX")
set(D3D12FRAME_ROOT_PATH "${D3D12PROJECT_ROOT_PATH}/D3DFrame")
set(D3D12X_ROOT_PATH "${D3D12PROJECT_ROOT_PATH}/directx")
set(D3D12XTK_ROOT_PATH "${D3D12PROJECT_ROOT_PATH}/DirectXTK")
set(D3D12FRAME_ASSIMP_LIB_ROOT_PATH "C:/Users/Administrator.PC-20191006TRUC/source/repos/Assimp/assimp")


set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /Subsystem:Windows")
add_definitions(-DUNICODE -D_UNICODE)

# 工程配置
option(D3D12FRAME_MODEL_LOADER "启用 Model 模块" OFF)
	
# 头文件与源文件

if (DEFINED D3D12FRAME_MODEL_LOADER)
	include_directories("${D3D12FRAME_ASSIMP_LIB_ROOT_PATH}/include")
	link_directories("${D3D12FRAME_ASSIMP_LIB_ROOT_PATH}/build/bin/Debug" "${D3D12FRAME_ASSIMP_LIB_ROOT_PATH}/build/lib/Debug")
endif()

include_directories(${D3D12FRAME_ROOT_PATH} ${D3D12PROJECT_ROOT_PATH} ${D3D12X_ROOT_PATH} "${D3D12XTK_ROOT_PATH}/includes")
file(GLOB ALL_SOURCES "${D3D12FRAME_ROOT_PATH}/*.cpp")

message(STATUS "${ALL_SOURCES}")

# 库
if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
	link_directories("${D3D12XTK_ROOT_PATH}/buildx64/debug")
else()
	link_directories("${D3D12XTK_ROOT_PATH}/build/debug")
endif()

add_library(D3D12_FRAME STATIC ${ALL_SOURCES})

add_executable(D3DApp Main.cpp ShadowMap.cpp SsaoMap.cpp D3DFrame.cpp)

target_link_libraries(D3DApp DirectXTK D3D12_FRAME)
