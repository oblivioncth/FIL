cmake_minimum_required(VERSION 3.21.1)

project(OFILb VERSION 0.4.1.3 LANGUAGES CXX)

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
set(PROJECT_SOURCES
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
    src/version.h
    src/main.cpp
    res/mainwindow.ui
    resources.qrc
    resources.rc
)

# Add executable target
qt_add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

# Set executable properties
set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE TRUE
)

# Link target to libraries
target_link_libraries(${PROJECT_NAME}
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
