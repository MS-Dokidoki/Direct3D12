@echo off
set rootFolder=%1
set buildFolder=%rootFolder% + /build

cd "%rootFolder%"

if not exist "build" ( 
    md build
    cd build
    cmake -A Win32 ..
) else (
    cd build
)
cmake --build .
cd "%rootFolder%"
@echo on