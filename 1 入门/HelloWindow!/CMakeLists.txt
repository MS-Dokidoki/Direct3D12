
project(HelloWindows!)
set(PROJECT_ROOT "C:\\Users\\Administrator.PC-20191006TRUC\\source\\repos\\DirectX")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SubSystem:Windows")

include_directories(${PROJECT_ROOT})
add_executable(D3DApp Win32.cpp D3DApp.cpp D3DHelper.cpp Main.cpp)