cmake_minimum_required(VERSION 3.16)
project(Clarity VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6 packages
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Quick Network)

# Core library (shared between all modes)
add_library(ClarityCore STATIC
    src/Core/Slide.h
    src/Core/Slide.cpp
    src/Core/Presentation.h
    src/Core/Presentation.cpp
    src/Core/PresentationModel.h
    src/Core/PresentationModel.cpp
    src/Core/IpcServer.h
    src/Core/IpcServer.cpp
    src/Core/IpcClient.h
    src/Core/IpcClient.cpp
    src/Core/SettingsManager.h
    src/Core/SettingsManager.cpp
)

target_link_libraries(ClarityCore
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Network
)

target_include_directories(ClarityCore
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)

# Main executable
add_executable(Clarity
    src/Main.cpp

    # Control app files
    src/Control/ControlMain.h
    src/Control/ControlMain.cpp
    src/Control/ControlWindow.h
    src/Control/ControlWindow.cpp
    src/Control/ProcessManager.h
    src/Control/ProcessManager.cpp
    src/Control/SettingsDialog.h
    src/Control/SettingsDialog.cpp
    src/Control/SlideEditorDialog.h
    src/Control/SlideEditorDialog.cpp

    # Output display files
    src/Output/OutputMain.h
    src/Output/OutputMain.cpp
    src/Output/OutputDisplay.h
    src/Output/OutputDisplay.cpp

    # Confidence monitor files
    src/Confidence/ConfidenceMain.h
    src/Confidence/ConfidenceMain.cpp

    # Resources
    resources/Resources.qrc
)

target_link_libraries(Clarity
    PRIVATE
        ClarityCore
        Qt6::Gui
        Qt6::Widgets
        Qt6::Quick
        Qt6::Network
)

# Platform-specific settings
if(WIN32)
    set_target_properties(Clarity PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# Install target
install(TARGETS Clarity
    RUNTIME DESTINATION bin
)
