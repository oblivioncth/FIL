#================= Project Setup ==========================

# CMake
cmake_minimum_required(VERSION 3.24.0)

# Project
# NOTE: DON'T USE TRAILING ZEROS IN VERSIONS
project(FIL
    VERSION 0.6.1
    LANGUAGES CXX
    DESCRIPTION "Flashpoint Importer for Launchers"
)

# Get helper scripts
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FetchOBCMake.cmake)
fetch_ob_cmake("202d606507f26a95c10d15ea170c7aed9e5c5341")

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
    REF "5b184c1ccea191ae6a885ea9a240a0d15b77b08c"
    COMPONENTS
        ${FIL_QX_COMPONENTS}
)

# Fetch libfp (build and import from source)
include(OB/Fetchlibfp)
ob_fetch_libfp("46d4068df217d7b6508c8b5afde10dd9a84fdfc5")

# Fetch CLIFp (build and import from source), force static build
set(BUILD_SHARED_LIBS OFF) # Shadows CACHE variable
include(OB/FetchCLIFp)
ob_fetch_clifp("2cd32b25b0d87e0913932bf62c38ca2b49fd5f31")
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
            TARGET "${PROJECT_NAMESPACE}::${APP_ALIAS_NAME}" COMPONENT "${APP_ALIAS_NAME}" DEFAULT
)


#================= Install ==========================

ob_standard_project_misc_install()

#====================== CPack ==============================

include(OB/Packaging)
ob_standard_project_package(VENDOR "oblivioncth")
