# ninwang: we do not use Eigen
cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 17)

add_library(mshloader STATIC MshLoader.cxx MshLoader.h) 
set_target_properties(mshloader PROPERTIES "COMPILE_DEFINITIONS" TETLIBRARY)