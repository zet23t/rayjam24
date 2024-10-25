if "%1" == "all" (
    del *.o
    cd ..\..\raylib\src
    make clean
    make PLATFORM=PLATFORM_WIN32 BUILD_MODE=RELEASE RAYLIB_LIBTYPE=SHARED
    cd - >nul
)

make RAYLIB_LIBTYPE=SHARED BUILD_MODE=DEBUG

raylib_game.exe 