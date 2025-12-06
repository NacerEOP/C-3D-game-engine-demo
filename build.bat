@echo off
echo Building 3D Game Engine (incremental preferred)...

REM Configure these paths (same as Makefile expects)
set SFML_PATH=C:/Users/NITRO/Documents/SFML Download/SFML-3.0.2-windows-gcc-14.2.0-mingw-64-bit/SFML-3.0.2
set ASSIMP_INCLUDE=C:/Users/NITRO/Documents/ASSIMP/assimp-headers
set ASSIMP_LIB=C:/Users/NITRO/Documents/MyGameEngine

REM If GNU make (mingw32-make) is available, use it (always rebuild .exe if any source changed)
where mingw32-make >nul 2>&1
if %errorlevel% == 0 (
    echo Found mingw32-make -> building using Makefile
    REM Delete game.exe so make will always rebuild it if sources changed
    del /Q game.exe 2>nul
    mingw32-make -j %NUMBER_OF_PROCESSORS%
    if %errorlevel% == 0 (
        echo.
        echo  BUILD SUCCESSFUL!
        goto :end
    ) else (
        echo.
        echo  mingw32-make failed with error %errorlevel% - falling back to full compile
    )
) else (
    echo mingw32-make not found in PATH
)

where make >nul 2>&1
if %errorlevel% == 0 (
    echo Found make -> building using Makefile
    REM Delete game.exe so make will always rebuild it if sources changed
    del /Q game.exe 2>nul
    make -j %NUMBER_OF_PROCESSORS%
    if %errorlevel% == 0 (
        echo.
        echo  BUILD SUCCESSFUL!
        goto :end
    ) else (
        echo.
        echo  make failed with error %errorlevel% - falling back to full compile
    )
) else (
    echo make not found in PATH
)

echo Performing full compile (fallback)
echo Compiling source files...

g++ main.cpp Core/Application.cpp Core/Engine.cpp Core/Time.cpp Graphics/Renderer.cpp Graphics/Camera.cpp Graphics/ShaderManager.cpp Graphics/Model.cpp Graphics/Light.cpp Graphics/Mesh.cpp Graphics/ManualMesh.cpp Graphics/TextureManager.cpp Graphics/DebugQuad.cpp Input/InputSystem.cpp Scene/Scene.cpp Scene/GameObject.cpp src/glad.c src/stb_image_impl.cpp -o game.exe -I"%SFML_PATH%/include" -I"%ASSIMP_INCLUDE%" -I"include" -L"%SFML_PATH%/lib" -L"%ASSIMP_LIB%" -lsfml-graphics -lsfml-window -lsfml-system "%ASSIMP_LIB%/libassimp.dll.a" -lopengl32 -lgdiplus -lole32 -static -static-libgcc -static-libstdc++ -std=c++17

if %errorlevel% == 0 (
    echo.
    echo  BUILD SUCCESSFUL!
    echo Build completed. Not running the executable automatically.
    echo.
    rem To run the game manually, execute: game.exe
    goto :end
) else (
    echo.
    echo  BUILD FAILED with error code %errorlevel%
    echo.
    pause
    exit /b %errorlevel%
)

:end
echo Done.