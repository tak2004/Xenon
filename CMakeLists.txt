cmake_minimum_required(VERSION 2.8.11)
project(Xenon)

include("Integrate.cmake")

# map all linker outputs to dist directory
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${Xenon_SOURCE_DIR}/dist)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${Xenon_SOURCE_DIR}/dist)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${Xenon_SOURCE_DIR}/dist)

# add an additional location, configure and integrate the package
rcf_addlocation(RADONFRAMEWORK "git" "https://github.com/tak2004/RadonFramework.git")
set(RADONFRAMEWORK_BUILD_UNITTEST OFF CACHE BOOL "" FORCE)
set(RADONFRAMEWORK_BUILD_SYSTEMTEST OFF CACHE BOOL "" FORCE)
set(RADONFRAMEWORK_BUILD_RESOURCETEST OFF CACHE BOOL "" FORCE)
Integrate(RADONFRAMEWORK "Radon framework" "RADONFRAMEWORK_HOME")

# create a static library
AddSourceDirectoryRecursive(srclist "src/" "Source")
AddHeaderDirectoryRecursive(hdrlist "include/Xenon" "Includes")
set(LIBXENON_FILES ${srclist} ${hdrlist})
Generate(MODULE LIBXENON LibXenon "Framework")
target_include_directories(LibXenon PRIVATE src/)
AddDependency(LIBXENON RadonFramework)
AddPublicInclude(LIBXENON "${Xenon_SOURCE_DIR}/include")
Finalize(LIBXENON)

# create an command line tool which use the static library
AddSourceDirectoryRecursive(filelist "tool" "Source")
set(XENON_FILES ${filelist})
Generate(EXECUTABLE XENON Xenon "Tools")
target_include_directories(Xenon PRIVATE src/)
AddDependency(XENON RadonFramework LibXenon)
Finalize(XENON)