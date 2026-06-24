project(vellum VERSION 0.1.0 LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(
  cxxopts
  GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
  GIT_TAG        v3.3.1
)
FetchContent_MakeAvailable(cxxopts)

add_executable(vellum main.cpp)
target_link_libraries(vellum PRIVATE vellum_compiler)
target_include_directories(vellum PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/vellum-compiler
    ${cxxopts_SOURCE_DIR}/include)
target_compile_definitions(vellum PRIVATE
    "VELLUM_VERSION=\"${PROJECT_VERSION}\"")
vellum_target_sentry(vellum "vellum@${PROJECT_VERSION}")
