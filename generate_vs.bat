@echo off
:: Set your vcpkg path here!
set VCPKG_ROOT=C:\dev\vcpkg

if not exist build mkdir build

echo Generating Visual Studio 2026 Solution...
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake

if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed.
    pause
    exit /b %errorlevel%
)

echo.
echo Success! Opening solution...
start build/vulkan-window.slnx