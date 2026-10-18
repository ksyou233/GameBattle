@echo off
rem ===========================================================================
rem  一键编译并运行（Windows）
rem  依赖：MinGW-w64 的 g++（需支持 C++17）
rem ===========================================================================
chcp 65001 >nul
setlocal

set "GXX=g++"
where g++ >nul 2>nul
if errorlevel 1 (
    if exist "D:\msys64\mingw64\bin\g++.exe" (
        set "GXX=D:\msys64\mingw64\bin\g++.exe"
    ) else (
        echo [错误] 未找到 g++，请先安装 MinGW-w64 并加入 PATH。
        pause
        exit /b 1
    )
)

if not exist build mkdir build

echo [1/2] 正在编译 ...
"%GXX%" -std=c++17 -O2 -Wall -Iinclude ^
    src\core\*.cpp src\character\*.cpp src\skill\*.cpp src\equipment\*.cpp ^
    src\combat\*.cpp src\observer\*.cpp src\battle\*.cpp main.cpp ^
    -o build\GameBattle.exe

if errorlevel 1 (
    echo [失败] 编译未通过。
    pause
    exit /b 1
)
echo [2/2] 编译成功，开始运行 ...
echo.

rem --no-pause：窗口保留由本脚本统一负责，程序自身不必再等回车
build\GameBattle.exe --no-pause

rem ---------------------------------------------------------------------------
rem  在这里暂停一次。
rem  双击运行本脚本时，命令行窗口会在脚本结束后立刻关闭，
rem  上百行演示输出会随窗口一起消失；pause 会等到按键后才退出，
rem  保证输出能被完整看完。若是在已有的 cmd 窗口里手动运行，
rem  多按一次回车即可。
rem ---------------------------------------------------------------------------
echo.
pause

endlocal
