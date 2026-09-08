
add_executable(simple_state_machine)
target_sources(simple_state_machine PRIVATE simple_state_machine.cpp)
target_compile_options(simple_state_machine PRIVATE -Wall -Wextra -Wpedantic -Werror)
target_link_libraries(simple_state_machine PRIVATE hfsm::hfsm)

add_executable(hierarchical_state_machine)
target_sources(hierarchical_state_machine PRIVATE hierarchical_state_machine.cpp)
target_compile_options(hierarchical_state_machine PRIVATE -Wall -Wextra -Wpedantic -Werror)
target_link_libraries(hierarchical_state_machine PRIVATE hfsm::hfsm)

add_executable(pseudo_state)
target_sources(pseudo_state PRIVATE pseudo_state.cpp)
target_compile_options(pseudo_state PRIVATE -Wall -Wextra -Wpedantic -Werror)
target_link_libraries(pseudo_state PRIVATE hfsm::hfsm)