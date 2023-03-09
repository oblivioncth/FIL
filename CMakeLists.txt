#================= Project Setup ==========================

# CMake
cmake_minimum_required(VERSION 3.24.0)

# Project
# NOTE: DON'T USE TRAILING ZEROS IN VERSIONS
project(FIL
    VERSION 0.6.0.3
    LANGUAGES CXX
    DESCRIPTION "Flashpoint Importer for Launchers"
)

# Get helper scripts
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FetchOBCMake.cmake)
fetch_ob_cmake("990d4c4a3987722a173f948b59fd45ce924a7d9c")

# Initialize project according to standard rules
include(OB/Project)
ob_standard_project_setup()

# Additional Project Variables
set(TARGET_FP_VERSION_PREFIX 11.1)

# Configuration options
# Handled by fetched libs, but set this here formally since they aren't part of the main project
option(BUILD_SHARED_LIBS "Build FIL with shared libraries" OFF)

# C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build augmentation
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
enable_language("RC")

#================= Build =========================

set(FIL_QT_COMPONENTS
    Core
    Gui
    Network
    Widgets
    Xml
    Sql
)

# Import Qt
add_compile_definitions(QT_DISABLE_DEPRECATED_BEFORE=0x060000)
find_package(Qt6 REQUIRED COMPONENTS ${FIL_QT_COMPONENTS})

# Fetch Qx (build and import from source)
set(FIL_QX_COMPONENTS
    Core
    Io
    Gui
    Network
    Widgets
    Windows-gui
    Xml
)

include(OB/FetchQx)
ob_fetch_qx(
    REF "a290fd23bb36779aeccf2ae881b542d22d716829"
    COMPONENTS
        ${FIL_QX_COMPONENTS}
)

# Fetch libfp (build and import from source)
include(OB/Fetchlibfp)
ob_fetch_libfp("07bf0d772769e38595459d44b71e52e80ef6d972")

# Fetch CLIFp (build and import from source), force static build
set(BUILD_SHARED_LIBS OFF) # Shadows CACHE variable
include(OB/FetchCLIFp)
ob_fetch_clifp("c4764af90f3bd6669ee45b0a8f261b38523429c8")
unset(BUILD_SHARED_LIBS) # Unshadow CACHE variable

# Fetch Neargye's Magic Enum
include(OB/FetchMagicEnum)
ob_fetch_magicenum("v0.8.0")

# Process Targets
set(APP_TARGET_NAME ${PROJECT_NAMESPACE_LC}_${PROJECT_NAMESPACE_LC})
set(APP_ALIAS_NAME ${PROJECT_NAMESPACE})
add_subdirectory(app)

#--------------------Package Config-----------------------

ob_standard_project_package_config(
    COMPATIBILITY "SameMinorVersion"
    CONFIG STANDARD
        TARGET_CONFIGS
            "${PROJECT_NAMESPACE}::${APP_ALIAS_NAME}"
        DEPENDS
            PACKAGE "Qt6" COMPONENTS ${FIL_QT_COMPONENTS}
            PACKAGE "Qx" VERSION ${Qx_VERSION} COMPONENTS ${FIL_QX_COMPONENTS}
)


#================= Install ==========================

ob_standard_project_misc_install()

#====================== CPack ==============================

include(OB/Packaging)
ob_standard_project_package(VENDOR "oblivioncth")
