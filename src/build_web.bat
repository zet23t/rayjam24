if "%1" == "all" (
    del *.o
    cd ..\..\raylib\src
    del *.o
    make clean
    make PLATFORM=PLATFORM_WEB BUILD_MODE=RELEASE
    cd - >nul
)
make PLATFORM=PLATFORM_WEB BUILD_MODE=RELEASE
emrun --no_browser --port 8080 .