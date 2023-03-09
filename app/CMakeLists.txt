# ------------------ Setup CLIFp as a resource acquisition --------------------------

# Setup copy of CLIFp executable to FIL generated resources directory after its built
# ...
# This statement creates a command that is known to be able to create the appropraite
# output file (CLIFP.exe) by copying it from the CLIFp build. The command depends on
# CLIFp having been built in-order to be invoked
set(CLIFP_RES_PATH "${CMAKE_CURRENT_BINARY_DIR}/res/file/CLIFp.exe")
add_custom_command(OUTPUT
        "${CLIFP_RES_PATH}"
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:CLIFp::CLIFp>
        "${CLIFP_RES_PATH}"
    DEPENDS CLIFp::CLIFp
)

# This statement creates a target that by itself does nothing, but since it depends
# on a file that the above command is known to be able to create, it will invoke
# the command automatically when run
add_custom_target(fil_copy_clifp
    ALL
    DEPENDS "${CLIFP_RES_PATH}"
)

# ------------------ Setup FIL --------------------------

# Add via ob standard executable
include(OB/Executable)
ob_add_standard_executable(${APP_TARGET_NAME}
    NAMESPACE "${PROJECT_NAMESPACE}"
    ALIAS "${APP_ALIAS_NAME}"
    OUTPUT_NAME "${APP_ALIAS_NAME}"
    SOURCE
        clifp.h
        clifp.cpp
        frontend/fe-data.h
        frontend/fe-data.cpp
        frontend/fe-installfoundation.h
        frontend/fe-installfoundation.cpp
        frontend/fe-install.h
        frontend/fe-install.cpp
        frontend/fe-items.h
        frontend/fe-items.cpp
        frontend/attractmode/am-data.h
        frontend/attractmode/am-data.cpp
        frontend/attractmode/am-install.h
        frontend/attractmode/am-install.cpp
        frontend/attractmode/am-items.h
        frontend/attractmode/am-items.cpp
        frontend/attractmode/am-settings-data.h
        frontend/attractmode/am-settings-data.cpp
        frontend/attractmode/am-settings-items.h
        frontend/attractmode/am-settings-items.cpp
        frontend/launchbox/lb-data.h
        frontend/launchbox/lb-data.cpp
        frontend/launchbox/lb-install.h
        frontend/launchbox/lb-install.cpp
        frontend/launchbox/lb-items.h
        frontend/launchbox/lb-items.cpp
        import-worker.h
        import-worker.cpp
        mainwindow.h
        mainwindow.cpp
        mainwindow.ui
        main.cpp
    RESOURCE "resources.qrc"
    LINKS
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
            Fp::Fp
            magic_enum::magic_enum
    CONFIG STANDARD
    WIN32
)

# Note that the executable depends on CLIFp being copied into its build dir
add_dependencies(${APP_TARGET_NAME} fil_copy_clifp)

# Generate resource collection for CLIFp
qt_add_resources(${APP_TARGET_NAME} "${PROJECT_NAMESPACE_LC}_clifp_res"
    PREFIX "/"
    BASE "${CMAKE_CURRENT_BINARY_DIR}/res"
    FILES
        "${CLIFP_RES_PATH}"
)

# Forward select project variables to C++ code
include(OB/CppVars)
ob_add_cpp_vars(${APP_TARGET_NAME}
    NAME "project_vars"
    PREFIX "PROJECT_"
    VARS
        FULL_NAME "\"${PROJECT_DESCRIPTION}\""
        SHORT_NAME "\"${PROJECT_NAME}\""
        TARGET_FP_VER_PFX_STR "\"${TARGET_FP_VERSION_PREFIX}\""
)

# Set target exe details
include(OB/WinExecutableDetails)
ob_set_win_executable_details(${APP_TARGET_NAME}
    ICON "res/app/FIL.ico"
    FILE_VER ${PROJECT_VERSION}
    PRODUCT_VER ${TARGET_FP_VERSION_PREFIX}
    COMPANY_NAME "oblivioncth"
    FILE_DESC "Flashpoint Importer for Launchers"
    INTERNAL_NAME "FIL"
    COPYRIGHT "Open Source @ 2021 oblivioncth"
    TRADEMARKS_ONE "All Rights Reserved"
    TRADEMARKS_TWO "GNU AGPL V3"
    ORIG_FILENAME "FIL.exe"
    PRODUCT_NAME "${PROJECT_DESCRIPTION}"
)
