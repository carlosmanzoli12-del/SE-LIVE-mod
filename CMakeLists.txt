cmake_minimum_required(VERSION 3.22...3.25)
project(streamforge VERSION 0.1.0 LANGUAGES C CXX)

add_library(streamforge MODULE)
target_sources(streamforge PRIVATE src/plugin-main.cpp)
target_compile_features(streamforge PRIVATE cxx_std_17)
target_include_directories(streamforge PRIVATE "${CMAKE_SOURCE_DIR}/UI/obs-frontend-api")
set_target_properties(streamforge PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
target_link_libraries(streamforge PRIVATE OBS::libobs OBS::frontend-api Qt6::Widgets)
set_target_properties_obs(streamforge PROPERTIES FOLDER "plugins" PREFIX "")

if(OS_WINDOWS)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/streamforge.rc.in
                 ${CMAKE_CURRENT_BINARY_DIR}/streamforge.rc @ONLY)
  target_sources(streamforge PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/streamforge.rc)
endif()
