
add_executable(simple_state_machine)
target_sources(simple_state_machine PRIVATE simple_state_machine.cpp)
target_compile_options(simple_state_machine PRIVATE -Wall -Wextra -Wpedantic -Werror -O2 -g)
target_link_libraries(simple_state_machine PRIVATE hfsm::hfsm)

add_executable(hierarchical_state_machine)
target_sources(hierarchical_state_machine PRIVATE hierarchical_state_machine.cpp)
target_compile_options(hierarchical_state_machine PRIVATE -Wall -Wextra -Wpedantic -Werror -O2 -g)
target_link_libraries(hierarchical_state_machine PRIVATE hfsm::hfsm)