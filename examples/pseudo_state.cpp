#include <hfsm/state_machine.h>
#include <hfsm/state_machine_def.h>

#include <iostream>

struct Context {
    bool start_requested{false};
    bool fast_mode{false};
    bool reset_requested{false};
};

Context context;

enum class State {
    Idle,
    Decision,
    Fast,
    Slow,
};

struct Idle : hfsm::state<Idle> {
    void on_entry() { std::cout << "enter Idle\n"; }
    void on_update() { std::cout << "update Idle\n"; }
    void on_exit() { std::cout << "exit Idle\n"; }
};

struct Decision : hfsm::pseudo_state<Decision> {
    void on_entry() { std::cout << "enter Decision\n"; }
    void on_exit() { std::cout << "exit Decision\n"; }
};

struct Fast : hfsm::state<Fast> {
    void on_entry() { std::cout << "enter Fast\n"; }
    void on_update() { std::cout << "update Fast\n"; }
    void on_exit() { std::cout << "exit Fast\n"; }
};

struct Slow : hfsm::state<Slow> {
    void on_entry() { std::cout << "enter Slow\n"; }
    void on_update() { std::cout << "update Slow\n"; }
    void on_exit() { std::cout << "exit Slow\n"; }
};

struct Machine : hfsm::state_machine_def<Machine, State> {
    using IdleState = state_entry<Idle, State::Idle>;
    using DecisionState = state_entry<Decision, State::Decision>;
    using FastState = state_entry<Fast, State::Fast>;
    using SlowState = state_entry<Slow, State::Slow>;

    bool should_start() { return context.start_requested; }
    bool should_run_fast() { return context.fast_mode; }
    bool should_run_slow() { return !context.fast_mode; }
    bool should_reset() { return context.reset_requested; }

    void make_decision() { std::cout << "action: make decision\n"; }
    void start_fast() { std::cout << "action: select fast mode\n"; }
    void start_slow() { std::cout << "action: select slow mode\n"; }
    void reset() { std::cout << "action: reset\n"; }

    using initial_state = IdleState;

    using transition_table = std::tuple<
        transition<IdleState, DecisionState, &Machine::should_start, &Machine::make_decision>,
        transition<DecisionState, FastState, &Machine::should_run_fast, &Machine::start_fast>,
        transition<DecisionState, SlowState, &Machine::should_run_slow, &Machine::start_slow>,
        transition<FastState, IdleState, &Machine::should_reset, &Machine::reset>,
        transition<SlowState, IdleState, &Machine::should_reset, &Machine::reset>>;
};

int main() {
    hfsm::state_machine<Machine> sm;

    std::cout << "start\n";
    sm.start();

    std::cout << "\nselect fast mode\n";
    context.start_requested = true;
    context.fast_mode = true;
    sm.step();
    context.start_requested = false;

    std::cout << "\nreset\n";
    context.reset_requested = true;
    sm.step();
    context.reset_requested = false;

    std::cout << "\nselect slow mode\n";
    context.start_requested = true;
    context.fast_mode = false;
    sm.step();
    context.start_requested = false;

    return 0;
}