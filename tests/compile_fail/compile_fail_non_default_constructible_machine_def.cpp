#include <hfsm/state_machine.h>
#include <hfsm/state_machine_def.h>

#include <tuple>

enum class State {
  Idle,
};

struct Idle : hfsm::state<Idle> {};

struct Machine : hfsm::state_machine_def<Machine, State> {
  Machine() = delete;

  using IdleState = state_entry<Idle, State::Idle>;

  using initial_state = IdleState;
  using transition_table = std::tuple<>;
};

int main() {
  hfsm::state_machine<Machine> sm;
  (void)sm;
}