#include <gtest/gtest.h>
#include <hfsm/state_machine.h>
#include <string>
#include <vector>

struct PseudoContext {
  void reset() {
    start_requested = false;
    fast_mode = false;
    slow_mode = false;
    route_ready = false;

    idle_entry = 0;
    idle_update = 0;
    idle_exit = 0;

    decision_entry = 0;
    decision_update = 0;
    decision_exit = 0;

    route_entry = 0;
    route_update = 0;
    route_exit = 0;

    fast_entry = 0;
    slow_entry = 0;

    events.clear();
  }

  bool start_requested{false};
  bool fast_mode{false};
  bool slow_mode{false};
  bool route_ready{false};

  int idle_entry{0};
  int idle_update{0};
  int idle_exit{0};

  int decision_entry{0};
  int decision_update{0};
  int decision_exit{0};

  int route_entry{0};
  int route_update{0};
  int route_exit{0};

  int fast_entry{0};
  int slow_entry{0};

  std::vector<std::string> events;
};

PseudoContext context;

enum class MachineState { Idle, Decision, Route, Fast, Slow };

struct Idle : hfsm::state<Idle> {
  void on_entry() {
    ++context.idle_entry;
    context.events.push_back("Idle::on_entry");
  }

  void on_update() {
    ++context.idle_update;
    context.events.push_back("Idle::on_update");
  }

  void on_exit() {
    ++context.idle_exit;
    context.events.push_back("Idle::on_exit");
  }
};

struct Decision : hfsm::pseudo_state<Decision> {
  void on_entry() {
    ++context.decision_entry;
    context.events.push_back("Decision::on_entry");
  }

  void on_update() {
    ++context.decision_update;
    context.events.push_back("Decision::on_update");
  }

  void on_exit() {
    ++context.decision_exit;
    context.events.push_back("Decision::on_exit");
  }
};

struct Route : hfsm::pseudo_state<Route> {
  void on_entry() {
    ++context.route_entry;
    context.events.push_back("Route::on_entry");
  }

  void on_update() {
    ++context.route_update;
    context.events.push_back("Route::on_update");
  }

  void on_exit() {
    ++context.route_exit;
    context.events.push_back("Route::on_exit");
  }
};

struct Fast : hfsm::state<Fast> {
  void on_entry() {
    ++context.fast_entry;
    context.events.push_back("Fast::on_entry");
  }
};

struct Slow : hfsm::state<Slow> {
  void on_entry() {
    ++context.slow_entry;
    context.events.push_back("Slow::on_entry");
  }
};

struct PseudoMachine : hfsm::state_machine_def<PseudoMachine, MachineState> {
  using IdleState = state_entry<Idle, MachineState::Idle>;

  using DecisionState = state_entry<Decision, MachineState::Decision>;

  using RouteState = state_entry<Route, MachineState::Route>;

  using FastState = state_entry<Fast, MachineState::Fast>;

  using SlowState = state_entry<Slow, MachineState::Slow>;

  bool should_start() { return context.start_requested; }

  bool should_run_fast() { return context.fast_mode; }

  bool should_use_route() { return context.slow_mode; }

  bool route_ready() { return context.route_ready; }

  void enter_decision() { context.events.push_back("enter_decision"); }

  void start_fast() { context.events.push_back("start_fast"); }

  void enter_route() { context.events.push_back("enter_route"); }

  void start_slow() { context.events.push_back("start_slow"); }

  using initial_state = IdleState;

  using transition_table = hfsm::mpl::mp_list<
      transition<IdleState, DecisionState, &PseudoMachine::should_start, &PseudoMachine::enter_decision>,

      transition<DecisionState, FastState, &PseudoMachine::should_run_fast, &PseudoMachine::start_fast>,

      transition<DecisionState, RouteState, &PseudoMachine::should_use_route, &PseudoMachine::enter_route>,

      transition<RouteState, SlowState, &PseudoMachine::route_ready, &PseudoMachine::start_slow>>;
};

class PseudoStateTest : public ::testing::Test {
 protected:
  void SetUp() override { context.reset(); }
};

TEST_F(PseudoStateTest, ResolvesPseudoStateWithinSingleStep) {
  hfsm::state_machine<PseudoMachine> sm;

  sm.start();

  context.start_requested = true;
  context.fast_mode = true;

  sm.step();

  EXPECT_EQ(sm.current_state(), MachineState::Fast);

  EXPECT_EQ(context.decision_entry, 1);
  EXPECT_EQ(context.decision_exit, 1);
  EXPECT_EQ(context.fast_entry, 1);
}

TEST_F(PseudoStateTest, ResolvesMultiplePseudoStatesWithinSingleStep) {
  hfsm::state_machine<PseudoMachine> sm;

  sm.start();

  context.start_requested = true;
  context.slow_mode = true;
  context.route_ready = true;

  sm.step();

  EXPECT_EQ(sm.current_state(), MachineState::Slow);

  EXPECT_EQ(context.decision_entry, 1);
  EXPECT_EQ(context.decision_exit, 1);

  EXPECT_EQ(context.route_entry, 1);
  EXPECT_EQ(context.route_exit, 1);

  EXPECT_EQ(context.slow_entry, 1);
}

TEST_F(PseudoStateTest, ExecutesPseudoLifecycleInCorrectOrder) {
  hfsm::state_machine<PseudoMachine> sm;

  sm.start();
  context.events.clear();

  context.start_requested = true;
  context.fast_mode = true;

  sm.step();

  EXPECT_EQ(
      context.events,
      (std::vector<std::string>{
          "Idle::on_exit", "enter_decision", "Decision::on_entry", "Decision::on_exit", "start_fast", "Fast::on_entry"
      })
  );
}

TEST_F(PseudoStateTest, DoesNotCallPseudoOnUpdate) {
  hfsm::state_machine<PseudoMachine> sm;

  sm.start();

  context.start_requested = true;
  context.fast_mode = true;

  sm.step();

  EXPECT_EQ(context.decision_update, 0);
  EXPECT_EQ(context.route_update, 0);
}

TEST_F(PseudoStateTest, DoesNotRemainInPseudoStateAfterStep) {
  hfsm::state_machine<PseudoMachine> sm;

  sm.start();

  context.start_requested = true;
  context.fast_mode = true;

  sm.step();

  EXPECT_NE(sm.current_state(), MachineState::Decision);
  EXPECT_NE(sm.current_state(), MachineState::Route);
}

TEST_F(PseudoStateTest, ThrowsWhenPseudoStateHasNoAvailableTransition) {
  hfsm::state_machine<PseudoMachine> sm;

  sm.start();

  context.start_requested = true;

  EXPECT_THROW(sm.step(), std::logic_error);
}

TEST_F(PseudoStateTest, SelectsFirstAvailablePseudoTransition) {
  hfsm::state_machine<PseudoMachine> sm;

  sm.start();

  context.start_requested = true;

  // Both outgoing guards from Decision are true.
  context.fast_mode = true;
  context.slow_mode = true;
  context.route_ready = true;

  sm.step();

  // Decision -> Fast appears before Decision -> Route.
  EXPECT_EQ(sm.current_state(), MachineState::Fast);

  EXPECT_EQ(context.fast_entry, 1);
  EXPECT_EQ(context.route_entry, 0);
  EXPECT_EQ(context.slow_entry, 0);
}

TEST_F(PseudoStateTest, UpdatesNormalStateWhenNoTransitionIsAvailable) {
  hfsm::state_machine<PseudoMachine> sm;

  sm.start();

  sm.step();

  EXPECT_EQ(sm.current_state(), MachineState::Idle);
  EXPECT_EQ(context.idle_update, 1);

  EXPECT_EQ(context.decision_update, 0);
  EXPECT_EQ(context.route_update, 0);
}