cmake_minimum_required(VERSION 3.16)
project(Clarity VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6 packages
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Quick Network Sql Multimedia)

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
    src/Core/BibleDatabase.h
    src/Core/BibleDatabase.cpp
    src/Core/Song.h
    src/Core/Song.cpp
    src/Core/SongLibrary.h
    src/Core/SongLibrary.cpp
    src/Core/Theme.h
    src/Core/Theme.cpp
    src/Core/ThemeManager.h
    src/Core/ThemeManager.cpp
    src/Core/WheelEventFilter.h
    src/Core/WheelEventFilter.cpp
    src/Core/SlidePreviewRenderer.h
    src/Core/SlidePreviewRenderer.cpp
)

target_link_libraries(ClarityCore
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
        Qt6::Sql
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
    src/Control/ScriptureDialog.h
    src/Control/ScriptureDialog.cpp
    src/Control/SongLibraryDialog.h
    src/Control/SongLibraryDialog.cpp
    src/Control/SongEditorDialog.h
    src/Control/SongEditorDialog.cpp
    src/Control/ThemeEditorDialog.h
    src/Control/ThemeEditorDialog.cpp
    src/Control/ThemeSelectorDialog.h
    src/Control/ThemeSelectorDialog.cpp
    src/Control/SlideGridDelegate.h
    src/Control/SlideGridDelegate.cpp
    src/Control/LivePreviewWidget.h
    src/Control/LivePreviewWidget.cpp
    src/Control/LivePreviewPanel.h
    src/Control/LivePreviewPanel.cpp
    src/Control/ConfidencePreviewWidget.h
    src/Control/ConfidencePreviewWidget.cpp

    # Output display files
    src/Output/OutputMain.h
    src/Output/OutputMain.cpp
    src/Output/OutputDisplay.h
    src/Output/OutputDisplay.cpp

    # Confidence monitor files
    src/Confidence/ConfidenceMain.h
    src/Confidence/ConfidenceMain.cpp
    src/Confidence/ConfidenceDisplay.h
    src/Confidence/ConfidenceDisplay.cpp

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
        Qt6::Multimedia
)

# Platform-specific settings
if(WIN32)
    set_target_properties(Clarity PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# Copy Bible database to build directory
set(BIBLE_DB_SOURCE "${CMAKE_SOURCE_DIR}/data/bible.db")
set(DATA_DEST_DIR "$<TARGET_FILE_DIR:Clarity>/data")

add_custom_command(TARGET Clarity POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${DATA_DEST_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${BIBLE_DB_SOURCE}"
        "${DATA_DEST_DIR}/bible.db"
    COMMENT "Copying Bible database to build directory"
)

# Copy sample songs library to build directory
set(SONGS_SOURCE "${CMAKE_SOURCE_DIR}/samples/songs.json")
set(CONFIG_DEST_DIR "$<TARGET_FILE_DIR:Clarity>/config/Clarity")

add_custom_command(TARGET Clarity POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CONFIG_DEST_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SONGS_SOURCE}"
        "${CONFIG_DEST_DIR}/songs.json"
    COMMENT "Copying sample songs library to build directory"
)

# Install target
install(TARGETS Clarity
    RUNTIME DESTINATION bin
)

# Install Bible database
install(FILES "${BIBLE_DB_SOURCE}"
    DESTINATION bin/data
)

# Install sample songs
install(FILES "${SONGS_SOURCE}"
    DESTINATION bin/config/Clarity
)
