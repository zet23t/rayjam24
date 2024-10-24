if "%1" == "all" (
    cd ..\..\raylib\src
    make clean
    make PLATFORM=PLATFORM_WEB BUILD_MODE=RELEASE
    cd - >nul
)
make PLATFORM=PLATFORM_WEB BUILD_MODE=RELEASE