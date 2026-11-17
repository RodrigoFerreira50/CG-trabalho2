cmake_minimum_required(VERSION 3.14)
project(Aterragem CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

# -----------------------------------------------------------------------------
# Dependências: usa as que estiverem instaladas (macOS: brew install glfw glew glm,
# Linux: gestor de pacotes); senão descarrega-as e compila-as (ex.: no Windows).
# -----------------------------------------------------------------------------
include(FetchContent)

find_package(OpenGL REQUIRED)

# GLFW
find_package(glfw3 3.3 QUIET)
if(NOT glfw3_FOUND)
  message(STATUS "GLFW nao encontrado - a descarregar")
  set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE)
  FetchContent_MakeAvailable(glfw)
endif()

# GLEW
find_package(GLEW QUIET)
if(GLEW_FOUND)
  set(GLEW_TARGET GLEW::GLEW)
else()
  message(STATUS "GLEW nao encontrado - a descarregar")
  set(ONLY_LIBS ON CACHE BOOL "" FORCE)
  set(glew-cmake_BUILD_SHARED OFF CACHE BOOL "" FORCE)
  set(CMAKE_POLICY_VERSION_MINIMUM 3.5)   # o glew-cmake é antigo demais para o CMake 4
  if(APPLE)   # os SDKs recentes do macOS já não têm AGL, que o glew-cmake usa
    set(AGL_LIBRARY ${OPENGL_gl_LIBRARY} CACHE FILEPATH "" FORCE)
  endif()
  FetchContent_Declare(glew
    GIT_REPOSITORY https://github.com/Perlmint/glew-cmake.git
    GIT_TAG        glew-cmake-2.2.0
    GIT_SHALLOW    TRUE)
  FetchContent_MakeAvailable(glew)
  set(GLEW_TARGET libglew_static)          # já define GLEW_STATIC
endif()

# GLM (só headers)
find_package(glm QUIET)
if(TARGET glm::glm)
  set(GLM_TARGET glm::glm)
else()
  find_path(GLM_INCLUDE_DIR glm/glm.hpp)
  if(GLM_INCLUDE_DIR)
    add_library(glm_headers INTERFACE)
    target_include_directories(glm_headers INTERFACE ${GLM_INCLUDE_DIR})
    set(GLM_TARGET glm_headers)
  else()
    message(STATUS "GLM nao encontrado - a descarregar")
    FetchContent_Declare(glm
      GIT_REPOSITORY https://github.com/g-truc/glm.git
      GIT_TAG        1.0.1
      GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(glm)
    set(GLM_TARGET glm::glm)
  endif()
endif()

# -----------------------------------------------------------------------------
# Executável (os shaders e os assets ficam na pasta do projeto: o programa procura-a
# a partir da pasta do executável, por isso não é preciso copiá-los)
# -----------------------------------------------------------------------------
file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_executable(aterragem ${SOURCES})
target_include_directories(aterragem PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(aterragem PRIVATE glfw ${GLEW_TARGET} ${GLM_TARGET} OpenGL::GL)
target_compile_definitions(aterragem PRIVATE _USE_MATH_DEFINES)   # M_PI no Visual Studio

if(APPLE)
  target_compile_definitions(aterragem PRIVATE GL_SILENCE_DEPRECATION)
endif()
if(MSVC)
  target_compile_options(aterragem PRIVATE /utf-8)   # o título da janela tem acentos
  set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT aterragem)
  set_property(TARGET aterragem PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()
