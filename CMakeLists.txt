cmake_minimum_required(VERSION 3.21)
project(SnookerPlayer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

qt_add_executable(SnookerPlayer
    SnookerPlayer_Build6001/src/main.cpp
    SnookerPlayer_Build6001/src/UI/MainWindow.cpp
    SnookerPlayer_Build6001/src/UI/MainWindow.h

    # Moteur de jeu
    Frame.cpp
    Frame.h
    Match.cpp
    Match.h
    GameManager.cpp
    GameManager.h
    Player.cpp
    Player.h
    Ball.cpp
    Ball.h
    BallSet.cpp
    BallSet.h
    Referee.cpp
    Referee.h
    ShotHistory.cpp
    ShotHistory.h
    Shot.cpp
    Shot.h
    FoulRecord.h
)

target_include_directories(SnookerPlayer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(SnookerPlayer PRIVATE Qt6::Widgets)
