# ------------------ Setup CLIFp as a resource acquisition --------------------------

# Setup copy of CLIFp executable to FIL generated resources directory after its built
# ...
# This statement creates a command that is known to be able to create the appropraite
# output file (CLIFP.exe) by copying it from the CLIFp build. The command depends on
# CLIFp having been built in-order to be invoked
set(CLIFP_RES_PATH "${CMAKE_CURRENT_BINARY_DIR}/res/file/clifp")
add_custom_command(OUTPUT
        "${CLIFP_RES_PATH}"
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:CLIFp::FrontendGui>
        "${CLIFP_RES_PATH}"
    DEPENDS CLIFp::FrontendGui
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
    import/backup.h
    import/backup.cpp
    import/details.h
    import/details.cpp
    import/image.h
    import/image.cpp
    import/properties.h
    import/properties.cpp
    import/settings.h
    import/settings.cpp
    import/worker.h
    import/worker.cpp
    launcher/abstract/lr-data.h
    launcher/abstract/lr-data.tpp
    launcher/abstract/lr-install.h
    launcher/abstract/lr-install.tpp
    launcher/abstract/lr-registration.h
    launcher/abstract/lr-registration.cpp
    launcher/implementation/attractmode/am-data.h
    launcher/implementation/attractmode/am-data.tpp
    launcher/implementation/attractmode/am-data.cpp
    launcher/implementation/attractmode/am-install.h
    launcher/implementation/attractmode/am-install.cpp
    launcher/implementation/attractmode/am-install_win.cpp
    launcher/implementation/attractmode/am-install_linux.cpp
    launcher/implementation/attractmode/am-items.h
    launcher/implementation/attractmode/am-items.cpp
    launcher/implementation/attractmode/am-settings-data.h
    launcher/implementation/attractmode/am-settings-data.cpp
    launcher/implementation/attractmode/am-settings-items.h
    launcher/implementation/attractmode/am-settings-items.cpp
    launcher/implementation/attractmode/am-registration.h
    launcher/implementation/attractmode/am-registration.cpp
    launcher/implementation/emulationstation/es-data.h
    launcher/implementation/emulationstation/es-data.cpp
    launcher/implementation/emulationstation/es-install.h
    launcher/implementation/emulationstation/es-install.cpp
    launcher/implementation/emulationstation/es-install_win.cpp
    launcher/implementation/emulationstation/es-install_linux.cpp
    launcher/implementation/emulationstation/es-items.h
    launcher/implementation/emulationstation/es-items.cpp
    launcher/implementation/emulationstation/es-registration.h
    launcher/implementation/emulationstation/es-registration.cpp
    launcher/interface/lr-data-interface.h
    launcher/interface/lr-data-interface.cpp
    launcher/interface/lr-install-interface.h
    launcher/interface/lr-install-interface.cpp
    launcher/interface/lr-install-interface_win.cpp
    launcher/interface/lr-install-interface_linux.cpp
    launcher/interface/lr-items-interface.h
    launcher/interface/lr-items-interface.cpp
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
        launcher/implementation/launchbox/lb-data.h
        launcher/implementation/launchbox/lb-data.cpp
        launcher/implementation/launchbox/lb-install.h
        launcher/implementation/launchbox/lb-install.cpp
        launcher/implementation/launchbox/lb-items.h
        launcher/implementation/launchbox/lb-items.cpp
        launcher/implementation/launchbox/lb-registration.h
        launcher/implementation/launchbox/lb-registration.cpp
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
