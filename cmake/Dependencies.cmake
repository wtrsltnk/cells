CPMAddPackage(
    NAME spdlog
    GITHUB_REPOSITORY gabime/spdlog
    GIT_TAG v1.7.0
)

CPMAddPackage(
    NAME fmt
    GITHUB_REPOSITORY fmtlib/fmt
    GIT_TAG 7.1.0
)

CPMAddPackage(
    NAME glm
    GITHUB_REPOSITORY g-truc/glm
    GIT_TAG 0.9.9.7
)

CPMAddPackage(
    NAME glfw
    GITHUB_REPOSITORY glfw/glfw
    GIT_TAG 3.3.2
    OPTIONS
        "GLFW_BUILD_EXAMPLES Off"
        "GLFW_BUILD_TESTS Off"
        "GLFW_BUILD_DOCS Off"
)

CPMAddPackage(
    NAME rapidcsv
    GITHUB_REPOSITORY d99kris/rapidcsv
    VERSION 8.65
    DOWNLOAD_ONLY YES
)
