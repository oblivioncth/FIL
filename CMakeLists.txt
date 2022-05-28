cmake_minimum_required(VERSION 3.21.1)

# Project
project(OFILb VERSION 0.4.1.3 LANGUAGES CXX)
set(TARGET_FP_VERSION 10.1.0.3)
set(FORMAL_NAME "Oby's Flashpoint Importer for LaunchBox")

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
fetch_qx(ae48c53243bb1b2140196026bbc72aff3e56fbd9) # dev commit

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
    src/frontend/fe-data.h
    src/frontend/fe-data.cpp
    src/frontend/fe-install.h
    src/frontend/fe-install.cpp
    src/frontend/fe-items.h
    src/frontend/fe-items.cpp
    src/frontend/launchbox/lb-data.h
    src/frontend/launchbox/lb-data.cpp
    src/frontend/launchbox/lb-install.h
    src/frontend/launchbox/lb-install.cpp
    src/frontend/launchbox/lb-items.h
    src/frontend/launchbox/lb-items.cpp
    src/import-worker.h
    src/import-worker.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    src/main.cpp
)

set(QT_UI_RESOURCES
    src/mainwindow.ui
)

set(QT_RESOURCES
    res/resources.qrc
)

# Organize sources for IDE
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src"
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

# Forward select project variables to C++ code
include(ProjectVars)
set_cxx_project_vars(${TARGET_NAME}
    FULL_NAME "${FORMAL_NAME}"
    SHORT_NAME "${PROJECT_NAME}"
    TARGET_FP_VER ${TARGET_FP_VERSION}
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
    PRODUCT_NAME "${FORMAL_NAME}"
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
        Qx::Gui
        Qx::Network
        Qx::Widgets
        Qx::Windows
        Qx::Windows-gui
        Qx::Xml
)
