#include <gtest/gtest.h>
#include <hfsm/state_machine.h>
#include <string>
#include <vector>

// ============================================================
// Test context
// ============================================================

struct PseudoStateContext {
  void reset() {
    start_requested = false;
    fast_mode = false;
    slow_mode = false;

    idle_entry_count = 0;
    idle_exit_count = 0;

    decision_entry_count = 0;
    decision_exit_count = 0;

    fast_running_entry_count = 0;
    slow_running_entry_count = 0;

    events.clear();
  }

  bool start_requested{false};
  bool fast_mode{false};
  bool slow_mode{false};

  int idle_entry_count{0};
  int idle_exit_count{0};

  int decision_entry_count{0};
  int decision_exit_count{0};

  int fast_running_entry_count{0};
  int slow_running_entry_count{0};

  std::vector<std::string> events;
};

PseudoStateContext context;

// ============================================================
// State enum
// ============================================================

enum class RunningState { Idle, Decision, FastRunning, SlowRunning };

// ============================================================
// States
// ============================================================

struct Idle : hfsm::state<Idle> {
  void on_entry() {
    ++context.idle_entry_count;
    context.events.push_back("Idle::on_entry");
  }

  void on_exit() {
    ++context.idle_exit_count;
    context.events.push_back("Idle::on_exit");
  }
};

struct Decision : hfsm::pseudo_state<Decision> {
  void on_entry() {
    ++context.decision_entry_count;
    context.events.push_back("Decision::on_entry");
  }

  void on_exit() {
    ++context.decision_exit_count;
    context.events.push_back("Decision::on_exit");
  }
};

struct FastRunning : hfsm::state<FastRunning> {
  void on_entry() {
    ++context.fast_running_entry_count;
    context.events.push_back("FastRunning::on_entry");
  }
};

struct SlowRunning : hfsm::state<SlowRunning> {
  void on_entry() {
    ++context.slow_running_entry_count;
    context.events.push_back("SlowRunning::on_entry");
  }
};

// ============================================================
// State machine definition
// ============================================================

struct RunningMachine : hfsm::state_machine_def<RunningMachine, RunningState> {
  using IdleState = state_entry<Idle, RunningState::Idle>;
  using DecisionState = state_entry<Decision, RunningState::Decision>;
  using FastRunningState = state_entry<FastRunning, RunningState::FastRunning>;
  using SlowRunningState = state_entry<SlowRunning, RunningState::SlowRunning>;

  bool should_start() { return context.start_requested; }
  bool should_run_fast() { return context.fast_mode; }
  bool should_run_slow() { return context.slow_mode; }

  void enter_decision() { context.events.push_back("RunningMachine::enter_decision"); }

  void start_fast() { context.events.push_back("RunningMachine::start_fast"); }

  void start_slow() { context.events.push_back("RunningMachine::start_slow"); }

  using initial_state = IdleState;

  using transition_table = hfsm::mpl::mp_list<
      transition<IdleState, DecisionState, &RunningMachine::should_start, &RunningMachine::enter_decision>,
      transition<DecisionState, FastRunningState, &RunningMachine::should_run_fast, &RunningMachine::start_fast>,
      transition<DecisionState, SlowRunningState, &RunningMachine::should_run_slow, &RunningMachine::start_slow>>;
};

// ============================================================
// Fixture
// ============================================================

class PseudoStateTest : public ::testing::Test {
 protected:
  void SetUp() override { context.reset(); }
};

// ============================================================
// Initial state
// ============================================================

TEST_F(PseudoStateTest, StartsFromIdle) {
  hfsm::state_machine<RunningMachine> sm;

  sm.start();

  EXPECT_EQ(sm.current_state(), RunningState::Idle);
  EXPECT_EQ(context.idle_entry_count, 1);

  EXPECT_EQ(context.decision_entry_count, 0);
  EXPECT_EQ(context.decision_exit_count, 0);
}

// ============================================================
// Idle -> Decision -> FastRunning
// ============================================================

TEST_F(PseudoStateTest, ResolvesPseudoStateToFastRunningWithinSingleStep) {
  hfsm::state_machine<RunningMachine> sm;

  sm.start();

  context.start_requested = true;
  context.fast_mode = true;

  sm.step();

  EXPECT_EQ(sm.current_state(), RunningState::FastRunning);

  EXPECT_EQ(context.idle_exit_count, 1);

  EXPECT_EQ(context.decision_entry_count, 1);
  EXPECT_EQ(context.decision_exit_count, 1);

  EXPECT_EQ(context.fast_running_entry_count, 1);
  EXPECT_EQ(context.slow_running_entry_count, 0);
}

// ============================================================
// Idle -> Decision -> SlowRunning
// ============================================================

TEST_F(PseudoStateTest, ResolvesPseudoStateToSlowRunningWithinSingleStep) {
  hfsm::state_machine<RunningMachine> sm;

  sm.start();

  context.start_requested = true;
  context.slow_mode = true;

  sm.step();

  EXPECT_EQ(sm.current_state(), RunningState::SlowRunning);

  EXPECT_EQ(context.idle_exit_count, 1);

  EXPECT_EQ(context.decision_entry_count, 1);
  EXPECT_EQ(context.decision_exit_count, 1);

  EXPECT_EQ(context.fast_running_entry_count, 0);
  EXPECT_EQ(context.slow_running_entry_count, 1);
}

// ============================================================
// Lifecycle order
// ============================================================

TEST_F(PseudoStateTest, ExecutesPseudoStateLifecycleInCorrectOrder) {
  hfsm::state_machine<RunningMachine> sm;

  sm.start();

  context.events.clear();

  context.start_requested = true;
  context.fast_mode = true;

  sm.step();

  const std::vector<std::string> expected = {
      "Idle::on_exit",
      "RunningMachine::enter_decision",
      "Decision::on_entry",

      "Decision::on_exit",
      "RunningMachine::start_fast",
      "FastRunning::on_entry"
  };

  EXPECT_EQ(context.events, expected);
}

// ============================================================
// Pseudo state must not remain active
// ============================================================

TEST_F(PseudoStateTest, DoesNotRemainInPseudoStateAfterStep) {
  hfsm::state_machine<RunningMachine> sm;

  sm.start();

  context.start_requested = true;
  context.fast_mode = true;

  sm.step();

  EXPECT_NE(sm.current_state(), RunningState::Decision);

  EXPECT_EQ(sm.current_state(), RunningState::FastRunning);
}

// ============================================================
// No outgoing transition from pseudo state
// ============================================================

TEST_F(PseudoStateTest, ThrowsWhenPseudoStateHasNoAvailableTransition) {
  hfsm::state_machine<RunningMachine> sm;

  sm.start();

  context.start_requested = true;

  context.fast_mode = false;
  context.slow_mode = false;

  EXPECT_THROW(sm.step(), std::logic_error);
}

// ============================================================
// First available transition wins
// ============================================================

TEST_F(PseudoStateTest, SelectsFirstAvailablePseudoTransition) {
  hfsm::state_machine<RunningMachine> sm;

  sm.start();

  context.start_requested = true;

  // Both guards are true.
  context.fast_mode = true;
  context.slow_mode = true;

  sm.step();

  // FastRunning transition appears first in transition_table.
  EXPECT_EQ(sm.current_state(), RunningState::FastRunning);

  EXPECT_EQ(context.fast_running_entry_count, 1);
  EXPECT_EQ(context.slow_running_entry_count, 0);
}