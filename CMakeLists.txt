cmake_minimum_required(VERSION 3.21.1)

project(OFILb VERSION 0.4.1.3 LANGUAGES CXX)
set(TARGET_FP_VERSION 10.1.0.3)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build augmentation
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
enable_language("RC")

# Add local modules
set(PROJ_SCRIPTS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${PROJ_SCRIPTS_PATH}/module")

# Import Qt
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Xml
    Sql
)

# Disable deprecated code
add_compile_definitions(QT_DISABLE_DEPRECATED_BEFORE=0x060000)

# Fetch Qx (build and import from source)
include(FetchQx)
fetch_qx(v0.2)

# Set app sources
set(CXX_SOURCES
    src/clifp.h
    src/clifp.cpp
    src/flashpoint/fp-db.h
    src/flashpoint/fp-db.cpp
    src/flashpoint/fp-install.h
    src/flashpoint/fp-install.cpp
    src/flashpoint/fp-items.h
    src/flashpoint/fp-items.cpp
    src/flashpoint/fp-json.h
    src/flashpoint/fp-json.cpp
    src/flashpoint/fp-macro.h
    src/flashpoint/fp-macro.cpp
    src/import-worker.h
    src/import-worker.cpp
    src/launchbox/lb-install.h
    src/launchbox/lb-install.cpp
    src/launchbox/lb-xml.h
    src/launchbox/lb-xml.cpp
    src/launchbox/lb-items.h
    src/launchbox/lb-items.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    src/main.cpp
)

set(QT_UI_RESOURCES
    res/ui/mainwindow.ui
)

set(QT_RESOURCES
    res/resources.qrc
)

set(WINDOWS_RESOURCES
    res/resources.rc
)

# Organize sources for IDE
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/res/ui"
    PREFIX "UI Resources"
    FILES ${QT_UI_RESOURCES}
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/res"
    PREFIX "General Resources"
    FILES ${QT_RESOURCES}
)

# Add executable target
set(TARGET_NAME ${PROJECT_NAME})
qt_add_executable(${TARGET_NAME} WIN32)

# Add sources to target
target_sources(${TARGET_NAME}
    PRIVATE
        ${CXX_SOURCES}
        ${QT_UI_RESOURCES}
        ${QT_RESOURCES}
)

# Set target exe details
include(WinExecutableDetails)
set_win_executable_details(${TARGET_NAME}
    ICON "res/icon/OFILb.ico"
    FILE_VER ${PROJECT_VERSION}
    PRODUCT_VER ${TARGET_FP_VERSION}
    COMPANY_NAME "oblivioncth"
    FILE_DESC "Flashpoint Importer for LaunchBox"
    INTERNAL_NAME "OFILb"
    COPYRIGHT "Open Source @ 2021 oblivioncth"
    TRADEMARKS_ONE "All Rights Reserved"
    TRADEMARKS_TWO "GNU AGPL V3"
    ORIG_FILENAME "OFILb.exe"
    PRODUCT_NAME "Oby's Flashpoint Importer for LaunchBox"
)

# Link target to libraries
target_link_libraries(${TARGET_NAME}
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Xml
        Qt6::Sql
        Qx::Core
        Qx::Io
        Qx::Widgets
        Qx::Xml
        Qx::Windows
)
