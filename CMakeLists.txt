cmake_minimum_required(VERSION 3.20)
project(SnookerPlayer VERSION 3.0 LANGUAGES CXX)

# --- Standard C++20 ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Options ---
option(SNOOKER_BUILD_TESTS  "Compiler les tests unitaires"    ON)
option(SNOOKER_BUILD_SERVER "Compiler le serveur API REST"    ON)

# --- Sources du moteur snooker (Core) ---
set(CORE_SOURCES
    Ball.cpp
    BallSet.cpp
    Frame.cpp
    GameManager.cpp
    Match.cpp
    Player.cpp
    Referee.cpp
    Shot.cpp
    ShotHistory.cpp
    Tournament.cpp
    core/Table.cpp
)

set(CORE_HEADERS
    Ball.h
    BallSet.h
    Frame.h
    GameManager.h
    Header.h
    Match.h
    Player.h
    Referee.h
    Shot.h
    ShotHistory.h
    Tournament.h
    core/Table.h
)

# --- Bibliotheque Core ---
add_library(snooker_core STATIC ${CORE_SOURCES} ${CORE_HEADERS})
target_include_directories(snooker_core PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)
target_compile_features(snooker_core PUBLIC cxx_std_20)

if (MSVC)
    target_compile_options(snooker_core PRIVATE /W4 /utf-8)
else()
    target_compile_options(snooker_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# --- Bibliotheque Camera ---
set(CAMERA_SOURCES
    camera/CameraCalibration.cpp
    camera/CameraFusion.cpp
)
set(CAMERA_HEADERS
    camera/CameraConfig.h
    camera/CameraCalibration.h
    camera/CameraFusion.h
)

add_library(snooker_camera STATIC ${CAMERA_SOURCES} ${CAMERA_HEADERS})
target_include_directories(snooker_camera PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/camera
)
target_link_libraries(snooker_camera PUBLIC snooker_core)

# --- Executable Simulation (snooker_sim) ---
add_executable(snooker_sim SnookerPlayer_01.cpp)
target_link_libraries(snooker_sim PRIVATE snooker_core)
target_include_directories(snooker_sim PRIVATE ${CMAKE_SOURCE_DIR})

# --- Serveur API REST (optionnel) ---
if (SNOOKER_BUILD_SERVER)
    set(API_SOURCES
        api/ApiServer.cpp
        api/JsonSerializer.cpp
    )
    set(API_HEADERS
        api/ApiServer.h
        api/JsonSerializer.h
    )

    add_library(snooker_api STATIC ${API_SOURCES} ${API_HEADERS})
    target_include_directories(snooker_api PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/api
    )
    target_link_libraries(snooker_api PUBLIC snooker_core snooker_camera)

    # Lier ws2_32 sur Windows pour les sockets
    if (WIN32)
        target_link_libraries(snooker_api PUBLIC ws2_32)
    endif()

    add_executable(snooker_server api/main_server.cpp)
    target_link_libraries(snooker_server PRIVATE snooker_api)
    target_include_directories(snooker_server PRIVATE ${CMAKE_SOURCE_DIR})
endif()

# --- Tests unitaires ---
if (SNOOKER_BUILD_TESTS)
    enable_testing()

    add_executable(snooker_tests tests/test_snooker.cpp)
    target_link_libraries(snooker_tests PRIVATE
        snooker_core
        snooker_camera
    )
    if (SNOOKER_BUILD_SERVER)
        target_link_libraries(snooker_tests PRIVATE snooker_api)
    endif()
    target_include_directories(snooker_tests PRIVATE ${CMAKE_SOURCE_DIR})
    if (WIN32)
        target_link_libraries(snooker_tests PRIVATE ws2_32)
    endif()

    add_test(NAME SnookerUnitTests COMMAND snooker_tests)
    set_tests_properties(SnookerUnitTests PROPERTIES
        PASS_REGULAR_EXPRESSION "0 FAIL"
    )
endif()

# --- Affichage recapitulatif ---
message(STATUS "")
message(STATUS "=== Snooker Player 3.0 ===")
message(STATUS "  Compilateur       : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Standard C++      : ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build type        : ${CMAKE_BUILD_TYPE}")
message(STATUS "  Tests             : ${SNOOKER_BUILD_TESTS}")
message(STATUS "  Serveur API       : ${SNOOKER_BUILD_SERVER}")
message(STATUS "")
message(STATUS "Cibles disponibles :")
message(STATUS "  snooker_sim       - Simulation de jeu (mode console)")
if (SNOOKER_BUILD_SERVER)
    message(STATUS "  snooker_server    - Serveur API REST (port 8080)")
endif()
if (SNOOKER_BUILD_TESTS)
    message(STATUS "  snooker_tests     - Tests unitaires")
endif()
message(STATUS "")
