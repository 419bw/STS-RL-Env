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

set(ACTION_SOURCES
    src/action/Actions.cpp
    src/action/LambdaAction.cpp
)

set(POWER_SOURCES
    src/power/Powers.cpp
)

set(CHARACTER_SOURCES
    src/character/Character.cpp
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

set(PLAYER_ACTIONS_SOURCES
    src/action/PlayerActions.cpp
)

# 合并所有源文件
set(ALL_SOURCES
    ${CORE_SOURCES}
    ${EVENT_SOURCES}
    ${ACTION_SOURCES}
    ${POWER_SOURCES}
    ${CHARACTER_SOURCES}
    ${CARD_SOURCES}
    ${RELIC_SOURCES}
    ${GAMESTATE_SOURCES}
    ${FLOW_SOURCES}
    ${RULES_SOURCES}
    ${PLAYER_ACTIONS_SOURCES}
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
