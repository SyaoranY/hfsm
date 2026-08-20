cmake_minimum_required(VERSION 3.20)

project(hfsm_consumer LANGUAGES CXX)

find_package(hfsm CONFIG REQUIRED)

add_executable(hfsm_consumer main.cpp)

target_link_libraries(
  hfsm_consumer
  PRIVATE
    hfsm::hfsm
)