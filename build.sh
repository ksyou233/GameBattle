#!/usr/bin/env bash
# =============================================================================
#  一键编译并运行（Git Bash / Linux / macOS）
# =============================================================================
set -e

CXX=${CXX:-g++}
mkdir -p build

echo "[1/2] 正在编译 ..."
"$CXX" -std=c++17 -O2 -Wall -Iinclude \
    src/core/*.cpp src/character/*.cpp src/skill/*.cpp src/equipment/*.cpp \
    src/combat/*.cpp src/observer/*.cpp src/battle/*.cpp main.cpp \
    -o build/GameBattle.exe

echo "[2/2] 编译成功，开始运行 ..."
echo
# --no-pause：终端里本来就看得见输出，且脚本调用不能被回车阻塞
./build/GameBattle.exe --no-pause
