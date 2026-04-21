cmake_minimum_required(VERSION 3.10)
project(STS_CPP VERSION 1.0.0 LANGUAGES CXX)

# ==========================================
# 编译选项配置
# ==========================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 添加编译警告
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ==========================================
# 源文件收集
# ==========================================
set(CORE_SOURCES
    # 核心模块无.cpp文件，仅头文件
)

set(EVENT_SOURCES
    # 事件模块无.cpp文件，仅头文件
)

set(ENGINE_SOURCES
    src/engine/GameEngine.cpp
)

set(STATE_SOURCES
    src/state/RunState.cpp
    src/state/CombatState.cpp
)

set(ACTION_SOURCES
    src/action/ActionManager.cpp
    src/action/Actions.cpp
    src/action/PlayerActions.cpp
    src/action/LambdaAction.cpp
)

set(SYSTEM_SOURCES
    src/system/DeckSystem.cpp
)

set(CHARACTER_SOURCES
    src/character/Character.cpp
    src/character/Monster.cpp
    src/character/monster/JawWorm.cpp
)

set(INTENT_SOURCES
    src/intent/Intent.cpp
    src/intent/IntentBrain.cpp
    src/intent/FixedBrain.cpp
    src/intent/RandomBrain.cpp
    src/intent/AdaptiveBrain.cpp
    src/intent/brains/JawWormBrain.cpp
)

set(CARD_SOURCES
    src/card/Cards.cpp
)

set(RELIC_SOURCES
    src/relic/Relics.cpp
    src/relic/AbstractRelic.cpp
)

set(GAMESTATE_SOURCES
    src/gamestate/GameState.cpp
)

set(FLOW_SOURCES
    src/flow/CombatFlow.cpp
)

set(RULES_SOURCES
    src/rules/BasicRules.cpp
)

set(POWER_SOURCES
    src/power/Powers.cpp
)

set(POTION_SOURCES
    src/potion/Potions.cpp
)

set(MAP_SOURCES
    src/map/MapGenerator.cpp
    src/map/MapRenderer.cpp
)

# 合并所有源文件（不含 main.cpp，供库和模块复用）
set(ENGINE_LIB_SOURCES
    ${ENGINE_SOURCES}
    ${STATE_SOURCES}
    ${ACTION_SOURCES}
    ${SYSTEM_SOURCES}
    ${CHARACTER_SOURCES}
    ${INTENT_SOURCES}
    ${CARD_SOURCES}
    ${RELIC_SOURCES}
    ${GAMESTATE_SOURCES}
    ${FLOW_SOURCES}
    ${RULES_SOURCES}
    ${POWER_SOURCES}
    ${POTION_SOURCES}
    ${MAP_SOURCES}
)

set(ALL_SOURCES
    ${ENGINE_LIB_SOURCES}
    src/main.cpp
)

# ==========================================
# 可执行目标
# ==========================================
add_executable(STS_Game ${ALL_SOURCES})

# 包含目录配置 (使用项目根目录作为基础路径)
target_include_directories(STS_Game PRIVATE
    ${CMAKE_SOURCE_DIR}
)

# ==========================================
# RL 训练环境 (可选)
#
# 编译为 Python C 扩展模块 (.pyd)
# 需要: pip install pybind11
# 启用: cmake -DBUILD_RL_ENV=ON ..
# ==========================================
option(BUILD_RL_ENV "Build RL training environment (pybind11 module)" OFF)

if(BUILD_RL_ENV)
    find_package(pybind11 REQUIRED)

    set(RL_SOURCES
        src/rl/STSEnv.cpp
        src/rl/pybind_module.cpp
        ${ENGINE_LIB_SOURCES}
    )

    pybind11_add_module(sts_env ${RL_SOURCES})
    target_include_directories(sts_env PRIVATE ${CMAKE_SOURCE_DIR})

    if(MSVC)
        target_compile_options(sts_env PRIVATE /bigobj)
    endif()

    message(STATUS "RL training environment: ENABLED")
endif()

# ==========================================
# 模块库 (可选：用于模块化编译)
# ==========================================
# 如果需要将各模块编译为独立库，可取消以下注释

# add_library(CoreLib STATIC ${CORE_SOURCES})
# target_include_directories(CoreLib PUBLIC ${CMAKE_SOURCE_DIR})

# add_library(ActionLib STATIC ${ACTION_SOURCES})
# target_include_directories(ActionLib PUBLIC ${CMAKE_SOURCE_DIR})
# target_link_libraries(ActionLib PUBLIC CoreLib)

# add_library(PowerLib STATIC ${POWER_SOURCES})
# target_include_directories(PowerLib PUBLIC ${CMAKE_SOURCE_DIR})
# target_link_libraries(PowerLib PUBLIC CoreLib ActionLib)

# add_library(CharacterLib STATIC ${CHARACTER_SOURCES})
# target_include_directories(CharacterLib PUBLIC ${CMAKE_SOURCE_DIR})
# target_link_libraries(CharacterLib PUBLIC CoreLib PowerLib)

# add_library(CardLib STATIC ${CARD_SOURCES})
# target_include_directories(CardLib PUBLIC ${CMAKE_SOURCE_DIR})
# target_link_libraries(CardLib PUBLIC CoreLib ActionLib PowerLib CharacterLib)

# add_library(RelicLib STATIC ${RELIC_SOURCES})
# target_include_directories(RelicLib PUBLIC ${CMAKE_SOURCE_DIR})
# target_link_libraries(RelicLib PUBLIC CoreLib CardLib)

# add_library(GameStateLib STATIC ${GAMESTATE_SOURCES})
# target_include_directories(GameStateLib PUBLIC ${CMAKE_SOURCE_DIR})
# target_link_libraries(GameStateLib PUBLIC 
#     CoreLib EventLib ActionLib PowerLib CharacterLib CardLib RelicLib)

# target_link_libraries(STS_Game PRIVATE GameStateLib)

# ==========================================
# 打印配置信息
# ==========================================
message(STATUS "=== STS_CPP Build Configuration ===")
message(STATUS "CMake version: ${CMAKE_VERSION}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Output directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

# ==========================================
# 测试编译配置 (Catch2 local)
# ==========================================
set(CATCH2_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/third_party)
set(CATCH2_SOURCES ${CMAKE_SOURCE_DIR}/third_party/catch_amalgamated.cpp)

set(ENGINE_SOURCES
    src/engine/GameEngine.cpp
    src/state/RunState.cpp
    src/state/CombatState.cpp
    src/action/ActionManager.cpp
    src/action/Actions.cpp
    src/action/PlayerActions.cpp
    src/action/LambdaAction.cpp
    src/system/DeckSystem.cpp
    src/flow/CombatFlow.cpp
    src/rules/BasicRules.cpp
    src/power/Powers.cpp
    src/potion/Potions.cpp
    src/character/Character.cpp
    src/character/Monster.cpp
    src/character/monster/JawWorm.cpp
    src/intent/Intent.cpp
    src/intent/IntentBrain.cpp
    src/intent/FixedBrain.cpp
    src/intent/RandomBrain.cpp
    src/intent/AdaptiveBrain.cpp
    src/intent/brains/JawWormBrain.cpp
    src/card/Cards.cpp
    src/relic/Relics.cpp
    src/relic/AbstractRelic.cpp
    src/gamestate/GameState.cpp
    src/map/MapGenerator.cpp
    src/map/MapRenderer.cpp
)

set(TEST_SOURCES
    ${CATCH2_SOURCES}
    ${ENGINE_SOURCES}
    src/test/test_main.cpp
    src/test/unit/TestDamagePipeline.cpp
    src/test/unit/TestCardSelection.cpp
    src/test/unit/TestMonster.cpp
    src/test/unit/TestPotionSystem.cpp
    src/test/unit/TestRandomDamageAction.cpp
    src/test/unit/TestVulnerableRelic.cpp
    src/test/unit/TestMapRenderer.cpp
    src/test/unit/TestDeterminism.cpp
    src/test/unit/TestEventBus.cpp
    src/test/unit/TestActionManager.cpp
    src/test/unit/TestLambdaAction.cpp
    src/test/unit/TestDeckSystem.cpp
    src/test/integration/TestCombatFlow.cpp
    src/test/integration/TestFullRound.cpp
)

add_executable(STS_Tests ${TEST_SOURCES})

target_include_directories(STS_Tests PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CATCH2_INCLUDE_DIR}
)

include(CTest)
add_test(NAME STS_Tests COMMAND STS_Tests)

message(STATUS "Unit tests: ENABLED (local Catch2)")
