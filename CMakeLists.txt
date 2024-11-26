#================= Project Setup ==========================

# CMake
cmake_minimum_required(VERSION 3.24.0...3.30.0)

# Project
# NOTE: DON'T USE TRAILING ZEROS IN VERSIONS
project(FIL
    VERSION 0.7.5.5
    LANGUAGES CXX
    DESCRIPTION "Flashpoint Importer for Launchers"
)

# Get helper scripts
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FetchOBCMake.cmake)
fetch_ob_cmake("9f45dec8dc1cd09d99862b7b7ea335b361cf7ff7")

# Initialize project according to standard rules
include(OB/Project)
ob_standard_project_setup()

# Additional Project Variables
set(TARGET_FP_VERSION_PREFIX 13.0)

# Configuration options
# Handled by fetched libs, but set this here formally since they aren't part of the main project
option(BUILD_SHARED_LIBS "Build FIL with shared libraries" OFF)

# C++
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build augmentation
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    enable_language("RC")
endif()

#================= Build =========================

set(FIL_QT_COMPONENTS
    Core
    Core5Compat
    Gui
    Network
    Widgets
    Xml
    Sql
)

# Import Qt
include(OB/Qt)
ob_find_package_qt(REQUIRED COMPONENTS ${FIL_QT_COMPONENTS})

# Fetch Qx (build and import from source)
set(FIL_QX_COMPONENTS
    Core
    Io
    Gui
    Network
    Widgets
    Xml
)

if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    list(APPEND FIL_QX_COMPONENTS Windows-gui)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    list(APPEND FIL_QX_COMPONENTS Linux)
endif()

include(OB/FetchQx)
ob_fetch_qx(
    REF "0572d288936afd63ff6f5b6ce4b1fbfc0f03b0eb"
    COMPONENTS
        ${FIL_QX_COMPONENTS}
)

# Fetch libfp (build and import from source)
include(OB/Fetchlibfp)
ob_fetch_libfp("183a479d00235d332aa1046a9b5ba98f62699752")

# Fetch CLIFp (build and import from source)
include(OB/Utility)
ob_cache_project_version(CLIFp)
include(OB/FetchCLIFp)
ob_fetch_clifp("7139ae998b292eb595e751ba4cb8599230435358")

# TODO: The shared build of this is essentially useless as only the CLIFp executable
# is deployed, which only works if it's statically linked. There isn't a simple way
# to force a build of CLIFp static when building FIL shared as the libraries that
# they share will have already been locked into being built with shared linking
# by FIL since you can only FetchContent a dependency once. The only real way
# to get around this would be to change the entire project structure to a "super-build"
# using ExternalProject.
#
# An alternative could be to use preprocessor macros and the like to alter FIL's behavior
# when built as a shared library. It could instead build CLIFp into the same bin directory
# and then know to copy that exe along with the required DLLs into the target FP install
# when deploying it at runtime.

# Fetch Neargye's Magic Enum
include(OB/FetchMagicEnum)
ob_fetch_magicenum("v0.9.3")

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
