cmake_minimum_required(VERSION 3.15)
project(GameBattle LANGUAGES CXX)

# ---------------------------------------------------------------------------
#  游戏战斗与技能系统 —— 设计模式综合演示
#  C++17，无任何第三方依赖
#
#  说明：这里显式列出全部源文件，而不是用 file(GLOB_RECURSE)。
#  原因有二：
#    1) CMake 官方推荐显式列出源文件，避免新增文件后忘记重新配置；
#    2) 工程路径含中文时，GLOB 的 CONFIGURE_DEPENDS 机制在 MinGW Makefiles
#       生成器下会产生乱码路径导致构建失败。
# ---------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(GAME_BATTLE_SOURCES
    main.cpp

    # core —— 单例、事件总线
    src/core/GameConfig.cpp
    src/core/EventBus.cpp

    # character —— 抽象接口、具体角色、抽象工厂
    src/character/Character.cpp
    src/character/HeroPackageFactory.cpp

    # skill —— 组合模式
    src/skill/Skill.cpp

    # equipment —— 装饰器
    src/equipment/EquipmentDecorator.cpp

    # combat —— 策略、状态、责任链
    src/combat/CharacterState.cpp
    src/combat/AIStrategy.cpp
    src/combat/DamageChain.cpp

    # observer —— 观察者
    src/observer/BattleObserver.cpp

    # battle —— 战斗主循环
    src/battle/Battle.cpp
)

add_executable(GameBattle ${GAME_BATTLE_SOURCES})

target_include_directories(GameBattle PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

if (MSVC)
    # 中文源码与控制台输出统一按 UTF-8 处理
    target_compile_options(GameBattle PRIVATE /W4 /utf-8)
else()
    target_compile_options(GameBattle PRIVATE -Wall)
endif()

# 可执行文件统一输出到 build/ 下，保持工程根目录整洁
set_target_properties(GameBattle PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build")
