# Sets up CLIFp to be built/installed as an external project for use in the main project

# git_ref - Tag, branch name, or commit hash to retrieve. According to CMake docs,
#           a commit hash is preferred for speed and reliability

function(fetch_clifp git_ref)
    include(FetchContent)
    FetchContent_Declare(CLIFp
        GIT_REPOSITORY "https://github.com/oblivioncth/CLIFp"
        GIT_TAG ${git_ref}
    )
    FetchContent_MakeAvailable(CLIFp)
endfunction()
