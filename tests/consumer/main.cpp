#include <hfsm/state_machine.h>

enum class State {
    Idle,
    Running
};

struct Idle : hfsm::state<Idle> {};
struct Running : hfsm::state<Running> {};

struct Machine : hfsm::state_machine_def<Machine, State> {
    using IdleState = state_entry<Idle, State::Idle>;
    using RunningState = state_entry<Running, State::Running>;

    using initial_state = IdleState;

    bool start_requested() { return true; }

    void start_running() {}

    using transition_table = hfsm::mpl::mp_list<
        transition<
            IdleState,
            RunningState,
            &Machine::start_requested,
            &Machine::start_running>>;
};

int main() {
    hfsm::state_machine<Machine> machine;

    machine.start();

    if (machine.current_state() != State::Idle) {
        return 1;
    }

    machine.step();

    if (machine.current_state() != State::Running) {
        return 1;
    }

    return 0;
}