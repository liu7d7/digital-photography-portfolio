cmake_minimum_required(VERSION 3.20)

project(port)

# assume emscripten

set(CMAKE_EXECUTABLE_SUFFIX ".html")

macro(append_compiler_flags FLAGS)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAGS}")
endmacro()

macro(append_compiler_flags_debug FLAGS)
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${FLAGS}")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${FLAGS}")
endmacro()

macro(append_linker_flags FLAGS)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${FLAGS}")
endmacro()

macro(append_linker_flags_debug FLAGS)
  set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} ${FLAGS}")
endmacro()

macro(append_linker_flags_opts FLAGS)
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} ${FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} ${FLAGS}")
endmacro()

cmake_path(SET LIB_DIR NORMALIZE "${CMAKE_CURRENT_LIST_DIR}/wasm_webgpu/lib")
cmake_path(SET PROJECT_DIR NORMALIZE "${CMAKE_CURRENT_LIST_DIR}")
cmake_path(SET COMMON_DIR NORMALIZE "${CMAKE_CURRENT_LIST_DIR}/wasm_webgpu/samples/common")

append_compiler_flags("-Wall -Wno-missing-braces -o3")
append_compiler_flags_debug("-g3")
append_compiler_flags("-ferror-limit=100")

append_linker_flags("-Wall")
append_linker_flags_debug("-g3")
append_linker_flags_debug("-sASSERTIONS=1 -jsDWEBGPU_DEBUG=1")

append_linker_flags("--js-library ${LIB_DIR}/lib_webgpu.js")
append_linker_flags("--js-library ${COMMON_DIR}/lib_demo.js")
append_linker_flags("--js-library ${COMMON_DIR}/library_miniprintf.js")
append_linker_flags("--js-library ${PROJECT_DIR}/lib_dlbin.js")
append_linker_flags("-sMINIMAL_RUNTIME=2 -sTEXTDECODER=2 -sABORTING_MALLOC=0")
append_linker_flags("-sENVIRONMENT=web")
append_linker_flags("--shell-file \"${CMAKE_SOURCE_DIR}/shell.html\"")
append_linker_flags("-sTOTAL_STACK=16KB -sINITIAL_MEMORY=4MB")
append_linker_flags("-sALLOW_MEMORY_GROWTH=1 -Wno-pthreads-mem-growth")
append_linker_flags("-Wno-experimental")
append_linker_flags("-sUSE_SDL=0 -sFILESYSTEM=0 -sAUTO_JS_LIBRARIES=0 -sDISABLE_EXCEPTION_THROWING=1")
append_linker_flags("-lmath.js -lhtml5.js -lint53.js")
append_linker_flags("--closure-args=--externs=${LIB_DIR}/webgpu-closure-externs.js")

# append_compiler_flags("-sMEMORY64 -Wno-experimental")
# append_linker_flags("-sMEMORY64 -sINITIAL_MEMORY=6GB -Wno-experimental")
# append_linker_flags("-sINITIAL_MEMORY=16MB -sMALLOC=emmalloc")

# Manual debugging:
#append_linker_flags("-g2 -sSINGLE_FILE=1")

append_linker_flags_opts("--closure 1")

include_directories("${LIB_DIR}")
include_directories("${COMMON_DIR}")

add_library(webgpu STATIC ${LIB_DIR}/lib_webgpu.cpp ${LIB_DIR}/lib_webgpu_cpp20.cpp)

add_executable(index main.c images.c input.c anim.c font.c state.c desc.c)
target_link_libraries(index webgpu)


