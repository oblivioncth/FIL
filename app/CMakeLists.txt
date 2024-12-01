# ------------------ Setup CLIFp as a resource acquisition --------------------------

# Setup copy of CLIFp executable to FIL generated resources directory after its built
# ...
# This statement creates a command that is known to be able to create the appropraite
# output file (CLIFP.exe) by copying it from the CLIFp build. The command depends on
# CLIFp having been built in-order to be invoked
set(CLIFP_RES_PATH "${CMAKE_CURRENT_BINARY_DIR}/res/file/clifp")
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
set(FIL_SOURCE
    import/properties.h
    import/properties.cpp
    import/settings.h
    import/worker.h
    import/worker.cpp
    launcher/lr-data.h
    launcher/lr-data.cpp
    launcher/lr-installfoundation.h
    launcher/lr-installfoundation.cpp
    launcher/lr-installfoundation_win.cpp
    launcher/lr-installfoundation_linux.cpp
    launcher/lr-install.h
    launcher/lr-install.cpp
    launcher/lr-items.h
    launcher/lr-items.cpp
    launcher/attractmode/am-data.h
    launcher/attractmode/am-data.cpp
    launcher/attractmode/am-install.h
    launcher/attractmode/am-install.cpp
    launcher/attractmode/am-install_win.cpp
    launcher/attractmode/am-install_linux.cpp
    launcher/attractmode/am-items.h
    launcher/attractmode/am-items.cpp
    launcher/attractmode/am-settings-data.h
    launcher/attractmode/am-settings-data.cpp
    launcher/attractmode/am-settings-items.h
    launcher/attractmode/am-settings-items.cpp
    ui/mainwindow.h
    ui/mainwindow.cpp
    ui/mainwindow.ui
    ui/progresspresenter.h
    ui/progresspresenter.cpp
    kernel/controller.h
    kernel/controller.cpp
    kernel/clifp.h
    kernel/clifp.cpp
    main.cpp
)

if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    list(APPEND FIL_SOURCE
        launcher/launchbox/lb-data.h
        launcher/launchbox/lb-data.cpp
        launcher/launchbox/lb-install.h
        launcher/launchbox/lb-install.cpp
        launcher/launchbox/lb-items.h
        launcher/launchbox/lb-items.cpp
    )
endif()

set(FIL_LINKS
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
        Qx::Xml
        Fp::Fp
        magic_enum::magic_enum
)

if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    list(APPEND FIL_LINKS
        Qx::Windows
        Qx::Windows-gui
    )
endif()

# Add via ob standard executable
include(OB/Executable)
ob_add_standard_executable(${APP_TARGET_NAME}
    NAMESPACE "${PROJECT_NAMESPACE}"
    ALIAS "${APP_ALIAS_NAME}"
    SOURCE ${FIL_SOURCE}
    RESOURCE "resources.qrc"
    LINKS ${FIL_LINKS}
    CONFIG STANDARD
    WIN32
)

target_sources(fil_fil
  PRIVATE

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
        VERSION_STR "\"${PROJECT_VERSION}\""
        TARGET_FP_VER_PFX_STR "\"${TARGET_FP_VERSION_PREFIX}\""
        BUNDLED_CLIFP_VERSION "\"${CLIFp_VERSION}\""
)

# Set target exe details
if(CMAKE_SYSTEM_NAME STREQUAL Windows)
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
endif()
