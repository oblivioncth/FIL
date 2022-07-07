# function(add_generated_resources_collection target)
# Creates a qrc resource file in the given directory with the
# given resources and adds the file to the given target
# Form:
# add_generated_resources_collection(my_target
#   OUTPUT "file/path"
#   PREFIX "prefix"
#   FILES
#       PATH "folder/file1.ext" ALIAS "alias1"
#       PATH "folder/file2.ext" ALIAS "alias2"
#       PATH "folder/file3.ext" ALIAS "alias3"
# )
#
# Optional args:
# PREFIX, ALIAS (for each file)
#
# Defaults:
# OUTPUT: "${CMAKE_CURRENT_BINARY_DIR}/res"
# PREFIX: "/"

function(__parse_file_entry return)
    #---------------- Function Setup ----------------------
    # Const variables
    set(ALIAS_ENTRY_TEMPLATE "<file alias=\"@FILE_ENTRY_ALIAS@\">@FILE_ENTRY_PATH@</file>")
    set(ENTRY_TEMPLATE "<file>@FILE_ENTRY_PATH@</file>")

    # Additional Function inputs
    set(oneValueArgs
        PATH
        ALIAS
    )

    # Parse arguments
    cmake_parse_arguments(FILE_ENTRY "" "${oneValueArgs}" "" ${ARGN})

    # Validate input
    foreach(unk_val ${FILE_ENTRY_UNPARSED_ARGUMENTS})
        message(WARNING "Ignoring unrecognized parameter: ${unk_val}")
    endforeach()

    if(${FILE_ENTRY_KEYWORDS_MISSING_VALUES})
        foreach(missing_val ${FILE_ENTRY_KEYWORDS_MISSING_VALUES})
            message(ERROR "A value for '${missing_val}' must be provided")
        endforeach()
        message(FATAL_ERROR "Not all required values were present!")
    endif()

    # Handle defaults/undefineds
    if(NOT DEFINED FILE_ENTRY_PATH)
        message(FATAL_ERROR "A path for each file entry must be included!")
    endif()

    #---------------- Parse Entry ----------------------
    if(DEFINED FILE_ENTRY_ALIAS)
        string(CONFIGURE "${ALIAS_ENTRY_TEMPLATE}" PARSED_ENTRY @ONLY)
    else()
        string(CONFIGURE "${ENTRY_TEMPLATE}" PARSED_ENTRY @ONLY)
    endif()

    set(${return} "${PARSED_ENTRY}" PARENT_SCOPE)
endfunction()

function(__parse_file_entry_list return)

    # Working vars (not required to "initialize" in cmake, but here for clarity)
    set(HAVE_FIRST_PATH FALSE)
    set(ENTRY_ARGS "")
    set(PARSED_LIST "")
    set(PARSED_ENTRY "")

    # Build parsed entry list
    foreach(word ${ARGN})
        if("${word}" STREQUAL "PATH")
            if(${HAVE_FIRST_PATH})
                # Parse sub-list
                __parse_file_entry(PARSED_ENTRY ${ENTRY_ARGS})
                list(APPEND PARSED_LIST "${PARSED_ENTRY}")

                # Reset intermediate argument list
                set(ENTRY_ARGS "")
            else()
                set(HAVE_FIRST_PATH TRUE)
            endif()
        endif()

        list(APPEND ENTRY_ARGS ${word})
    endforeach()

    # Process last sub-list (above loop ends while populating final sub-list)
    __parse_file_entry(PARSED_ENTRY ${ENTRY_ARGS})
    list(APPEND PARSED_LIST "${PARSED_ENTRY}")

    # Concatenate items
    list(JOIN PARSED_LIST "\n\t\t" FULLY_PARSED)

    set(${return} "${FULLY_PARSED}" PARENT_SCOPE)
endfunction()

function(add_generated_resources_collection target)
    #---------------- Function Setup ----------------------

    # Const variables
    set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/res")
    set(GENERATED_NAME "resources.qrc")
    set(GENERATED_PATH "${GENERATED_DIR}/${GENERATED_NAME}")
    set(TEMPLATE_FILE "__resources.qrc.in")

    # Additional Function inputs
    set(oneValueArgs
        OUTPUT
        PREFIX
    )
    set(multiValueArgs
        FILES
    )

    # Parse arguments
    cmake_parse_arguments(GEN_RES "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Validate input
    foreach(unk_val ${GEN_RES_UNPARSED_ARGUMENTS})
        message(WARNING "Ignoring unrecognized parameter: ${unk_val}")
    endforeach()

    if(${GEN_RES_KEYWORDS_MISSING_VALUES})
        foreach(missing_val ${GEN_RES_KEYWORDS_MISSING_VALUES})
            message(ERROR "A value for '${missing_val}' must be provided")
        endforeach()
        message(FATAL_ERROR "Not all required values were present!")
    endif()

    # Handle defaults/undefineds
    if(NOT DEFINED GEN_RES_OUTPUT)
        set(GEN_RES_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/res")
    endif()
    if(NOT DEFINED GEN_RES_PREFIX)
        set(GEN_RES_PREFIX "/")
    endif()
    if(NOT DEFINED GEN_RES_FILES)
        message(FATAL_ERROR "A file list must be included!")
    endif()

    #---------------- Collection File Generation ----------------------

    # Set prefix for file configuration
    set(__RES_PREFIX ${GEN_RES_PREFIX})

    # Set file entries for file configuration
    __parse_file_entry_list(__RES_FILES ${GEN_RES_FILES})

    # Generate resources.qrc
    configure_file("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/${TEMPLATE_FILE}"
        "${GENERATED_PATH}"
        @ONLY
        NEWLINE_STYLE UNIX
    )

    # Add file to target
    target_sources(${target} PRIVATE "${GENERATED_PATH}")
endfunction()
