cmake_minimum_required(VERSION 3.21)
project(SnookerPlayer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

list(APPEND CMAKE_PREFIX_PATH
    "C:/Qt/6.11.1/msvc2022_64"
    "D:/Dev/vcpkg-master/vcpkg-master/installed/x64-windows"
)

find_package(Qt6 REQUIRED COMPONENTS Widgets TextToSpeech)
qt_standard_project_setup()

# ---------------------------------------------------------------------
# Module MesureVision : calibration camera (homographie / ArUco),
# detection et suivi des billes, assemblage des 3 images cameras.
# Independant de Qt : bibliotheque C++ + OpenCV pure, utilisable et
# testable (images statiques) sans avoir de camera physique branchee.
# ---------------------------------------------------------------------
find_package(OpenCV REQUIRED)

add_library(MesureVision STATIC
    CameraCalibration.cpp
    CameraCalibration.h
    BallDetector.cpp
    BallDetector.h
    BallTracker.cpp
    BallTracker.h
    BallMapRecorder.cpp
    BallMapRecorder.h
    TableCapture.cpp
    TableCapture.h
    ShotAnalyzer.cpp
    ShotAnalyzer.h
)
target_include_directories(MesureVision PUBLIC
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(MesureVision PUBLIC ${OpenCV_LIBS})

# Outil de calibration manuelle (clic souris sur les 4 coins d'une zone).
# Utilisation : CalibrationTool.exe <index_camera 0/1/2> <image.jpg ou index camera>
add_executable(CalibrationTool CalibrationTool.cpp)
target_link_libraries(CalibrationTool PRIVATE MesureVision)

# Outil de calibration automatique par marqueurs ArUco.
# Utilisation : ArucoCalibrationTool.exe --generate
#               ArucoCalibrationTool.exe <index_camera 0/1/2> <image.jpg ou index camera>
add_executable(ArucoCalibrationTool ArucoCalibrationTool.cpp)
target_link_libraries(ArucoCalibrationTool PRIVATE MesureVision)

# Outil de demo : detection + suivi + cartographie des billes sur une
# image statique (voir MesureVisionDemo.cpp).
# Utilisation : MesureVisionDemo.exe <image.png> [dossier_sortie]
add_executable(MesureVisionDemo MesureVisionDemo.cpp)
target_link_libraries(MesureVisionDemo PRIVATE MesureVision)

# Pont entre MesureVision et le moteur de jeu : transforme une sequence
# d'images (flux ou suite de photos statiques) en appels reels au moteur
# de jeu (Frame::playShot()/foul()), voir VisionGameBridge.h. Compile le
# moteur de jeu necessaire directement (independant de Qt) et relie
# MesureVision pour la detection/le suivi des billes.
add_executable(VisionGameBridgeDemo
    VisionGameBridgeDemo.cpp
    VisionGameBridge.cpp
    VisionGameBridge.h
    Frame.cpp
    Frame.h
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
target_link_libraries(VisionGameBridgeDemo PRIVATE MesureVision)

qt_add_executable(SnookerPlayer
    SnookerPlayer_Build6001/src/main.cpp
    SnookerPlayer_Build6001/src/UI/MainWindow.cpp
    SnookerPlayer_Build6001/src/UI/MainWindow.h
    SnookerPlayer_Build6001/src/UI/MoveLogWidget.cpp
    SnookerPlayer_Build6001/src/UI/MoveLogWidget.h
    SnookerPlayer_Build6001/src/UI/TestScenarioRunner.cpp
    SnookerPlayer_Build6001/src/UI/TestScenarioRunner.h
    SnookerPlayer_Build6001/src/UI/SpeechAnnouncer.cpp
    SnookerPlayer_Build6001/src/UI/SpeechAnnouncer.h
    SnookerPlayer_Build6001/src/UI/RulesReferenceDialog.cpp
    SnookerPlayer_Build6001/src/UI/RulesReferenceDialog.h
    SnookerPlayer_Build6001/src/Storage/MatchStorage.cpp
    SnookerPlayer_Build6001/src/Storage/MatchStorage.h

    # Pont vision -> moteur de jeu (suivi camera en direct)
    VisionGameBridge.cpp
    VisionGameBridge.h

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

target_link_libraries(SnookerPlayer PRIVATE Qt6::Widgets Qt6::TextToSpeech MesureVision)

# Chemin du texte du reglement officiel (voir RulesReferenceDialog),
# injecte en dur au chemin source : ce projet n'a pas encore d'etape
# de deploiement/packaging, tout tourne depuis l'arbre source en dev.
target_compile_definitions(SnookerPlayer PRIVATE
    RULES_TEXT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/docs/reglement_snooker/Regles_officielles_snooker_FR_sept2024.txt"
)