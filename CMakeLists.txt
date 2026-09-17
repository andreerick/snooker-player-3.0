cmake_minimum_required(VERSION 3.21)
project(SnookerPlayer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

if(WIN32)
    list(APPEND CMAKE_PREFIX_PATH
        "C:/Qt/6.11.1/msvc2022_64"
        "D:/Dev/vcpkg-master/vcpkg-master/installed/x64-windows"
    )
endif()

# Bundle .app macOS : sans lui, l'executable brut n'a pas d'Info.plist et
# macOS refuse silencieusement l'acces camera (pas de NSCameraUsageDescription),
# sans meme afficher la demande d'autorisation a l'utilisateur.
if(APPLE)
    set(MACOSX_BUNDLE_BUNDLE_NAME "SnookerPlayer")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.andreerick.snookerplayer")
    set(MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}")
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets TextToSpeech Network)
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
    SnookerPlayer_Build6001/src/UI/MatchWebServer.cpp
    SnookerPlayer_Build6001/src/UI/MatchWebServer.h
    SnookerPlayer_Build6001/src/UI/ShareSessionDialog.cpp
    SnookerPlayer_Build6001/src/UI/ShareSessionDialog.h
    SnookerPlayer_Build6001/src/UI/HomeScreen.cpp
    SnookerPlayer_Build6001/src/UI/HomeScreen.h
    SnookerPlayer_Build6001/src/UI/CueSenseLauncher.cpp
    SnookerPlayer_Build6001/src/UI/CueSenseLauncher.h
    SnookerPlayer_Build6001/src/UI/PlayersDialog.cpp
    SnookerPlayer_Build6001/src/UI/PlayersDialog.h
    SnookerPlayer_Build6001/src/UI/TournamentManager.cpp
    SnookerPlayer_Build6001/src/UI/TournamentManager.h
    SnookerPlayer_Build6001/src/UI/TournamentDialog.cpp
    SnookerPlayer_Build6001/src/UI/TournamentDialog.h
    SnookerPlayer_Build6001/src/UI/SettingsDialog.cpp
    SnookerPlayer_Build6001/src/UI/SettingsDialog.h
    SnookerPlayer_Build6001/src/UI/TutorialDialog.cpp
    SnookerPlayer_Build6001/src/UI/TutorialDialog.h
    SnookerPlayer_Build6001/src/UI/TrainingChoiceDialog.cpp
    SnookerPlayer_Build6001/src/UI/TrainingChoiceDialog.h
    SnookerPlayer_Build6001/src/UI/ExerciseDialog.cpp
    SnookerPlayer_Build6001/src/UI/ExerciseDialog.h
    SnookerPlayer_Build6001/src/UI/UiUtils.cpp
    SnookerPlayer_Build6001/src/UI/UiUtils.h

    # Bibliotheques tierces vendorees (voir third_party/) : serveur web
    # (cpp-httplib, header-only) et generateur de QR code (qrcodegen).
    third_party/httplib.h
    third_party/qrcodegen.hpp
    third_party/qrcodegen.cpp
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
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

target_link_libraries(SnookerPlayer PRIVATE Qt6::Widgets Qt6::TextToSpeech Qt6::Network MesureVision)

# ws2_32 : sockets Windows, necessaires a cpp-httplib (voir MatchWebServer)
# meme en HTTP simple (pas de HTTPS, donc pas besoin d'OpenSSL ici).
# Sur macOS/Linux, cpp-httplib utilise directement les sockets POSIX du systeme.
if(WIN32)
    target_link_libraries(SnookerPlayer PRIVATE ws2_32)
endif()

if(APPLE)
    set_target_properties(SnookerPlayer PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/macos/Info.plist.in"
    )
endif()

# Chemin du texte du reglement officiel (voir RulesReferenceDialog) et de
# l'image de fond de l'ecran d'accueil (voir HomeScreen), injectes en dur
# au chemin source : ce projet n'a pas encore d'etape de deploiement/
# packaging, tout tourne depuis l'arbre source en dev.
target_compile_definitions(SnookerPlayer PRIVATE
    RULES_TEXT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/docs/reglement_snooker/Regles_officielles_snooker_FR_sept2024.txt"
    HOME_SCREEN_IMAGE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/assets/accueil_v3.png"
)