
project(D3DAppFrame)
set(PROJECT_ROOT "C:\\Users\\Administrator.PC-20191006TRUC\\source\\repos\\DirectX")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /Subsystem:Windows")

add_definitions(-DUNICODE -D_UNICODE)

include_directories(${PROJECT_ROOT})
add_executable(D3DApp D3DApp.cpp D3DHelper.cpp GameTimer.cpp Main.cpp)