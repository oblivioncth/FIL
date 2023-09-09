#================= Project Setup ==========================

# CMake
cmake_minimum_required(VERSION 3.24.0...3.26.0)

# Project
# NOTE: DON'T USE TRAILING ZEROS IN VERSIONS
project(FIL
    VERSION 0.7.1.2
    LANGUAGES CXX
    DESCRIPTION "Flashpoint Importer for Launchers"
)

# Get helper scripts
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FetchOBCMake.cmake)
fetch_ob_cmake("v0.3.2")

# Initialize project according to standard rules
include(OB/Project)
ob_standard_project_setup()

# Additional Project Variables
set(TARGET_FP_VERSION_PREFIX 12.0)

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
    REF "v0.5.3"
    COMPONENTS
        ${FIL_QX_COMPONENTS}
)

# Fetch libfp (build and import from source)
include(OB/Fetchlibfp)
ob_fetch_libfp("v0.4.2.2")

# Fetch CLIFp (build and import from source)
include(OB/FetchCLIFp)
ob_fetch_clifp("v0.9.6")

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
