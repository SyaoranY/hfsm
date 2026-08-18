#include <gtest/gtest.h>

#include <string>
#include <tuple>
#include <vector>

#include <hfsm/state_machine.h>


// ============================================================
// Test context
// ============================================================

struct TestContext {
  void reset() {
    level1_entry_count = 0;
    level1_update_count = 0;
    level1_exit_count = 0;

    level2_entry_count = 0;
    level2_update_count = 0;
    level2_exit_count = 0;

    level3_entry_count = 0;
    level3_update_count = 0;
    level3_exit_count = 0;

    idle_entry_count = 0;
    idle_update_count = 0;
    idle_exit_count = 0;

    level1_done_entry_count = 0;

    finish_level1 = false;

    events.clear();
  }

  int level1_entry_count{0};
  int level1_update_count{0};
  int level1_exit_count{0};

  int level2_entry_count{0};
  int level2_update_count{0};
  int level2_exit_count{0};

  int level3_entry_count{0};
  int level3_update_count{0};
  int level3_exit_count{0};

  int idle_entry_count{0};
  int idle_update_count{0};
  int idle_exit_count{0};

  int level1_done_entry_count{0};

  bool finish_level1{false};

  std::vector<std::string> events;
};

TestContext context;


// ============================================================
// Level 3
// ============================================================

enum class Level3State {
  Idle,
  Running
};

struct Idle : hfsm::state<Idle> {
  void on_entry() {
    ++context.idle_entry_count;
    context.events.push_back("Idle::on_entry");
  }

  void on_update() {
    ++context.idle_update_count;
    context.events.push_back("Idle::on_update");
  }

  void on_exit() {
    ++context.idle_exit_count;
    context.events.push_back("Idle::on_exit");
  }
};

struct Running : hfsm::state<Running> {
};


struct Level3Machine
    : hfsm::state_machine_def<Level3Machine, Level3State> {

  using IdleState =
      state_entry<Idle, Level3State::Idle>;

  using RunningState =
      state_entry<Running, Level3State::Running>;

  void on_entry() {
    ++context.level3_entry_count;
    context.events.push_back("Level3Machine::on_entry");
  }

  void on_update() {
    ++context.level3_update_count;
    context.events.push_back("Level3Machine::on_update");
  }

  void on_exit() {
    ++context.level3_exit_count;
    context.events.push_back("Level3Machine::on_exit");
  }

  bool start_running() {
    return false;
  }

  void on_start_running() {
  }

  using initial_state = IdleState;

  using transition_table = std::tuple<
      transition<
          IdleState,
          RunningState,
          &Level3Machine::start_running,
          &Level3Machine::on_start_running
      >
  >;
};


// ============================================================
// Level 2
// ============================================================

enum class Level2State {
  Level3,
  Done
};

struct Level2Done : hfsm::state<Level2Done> {
};


struct Level2Machine
    : hfsm::state_machine_def<Level2Machine, Level2State> {

  using Level3StateRef =
      state_entry<Level3Machine, Level2State::Level3>;

  using DoneState =
      state_entry<Level2Done, Level2State::Done>;

  void on_entry() {
    ++context.level2_entry_count;
    context.events.push_back("Level2Machine::on_entry");
  }

  void on_update() {
    ++context.level2_update_count;
    context.events.push_back("Level2Machine::on_update");
  }

  void on_exit() {
    ++context.level2_exit_count;
    context.events.push_back("Level2Machine::on_exit");
  }

  bool finish() {
    return false;
  }

  void on_finish() {
  }

  using initial_state = Level3StateRef;

  using transition_table = std::tuple<
      transition<
          Level3StateRef,
          DoneState,
          &Level2Machine::finish,
          &Level2Machine::on_finish
      >
  >;
};


// ============================================================
// Level 1
// ============================================================

enum class Level1State {
  Level2,
  Done
};

struct Level1Done : hfsm::state<Level1Done> {
  void on_entry() {
    ++context.level1_done_entry_count;
    context.events.push_back("Level1Done::on_entry");
  }
};


struct Level1Machine
    : hfsm::state_machine_def<Level1Machine, Level1State> {

  using Level2StateRef =
      state_entry<Level2Machine, Level1State::Level2>;

  using DoneState =
      state_entry<Level1Done, Level1State::Done>;

  void on_entry() {
    ++context.level1_entry_count;
    context.events.push_back("Level1Machine::on_entry");
  }

  void on_update() {
    ++context.level1_update_count;
    context.events.push_back("Level1Machine::on_update");
  }

  void on_exit() {
    ++context.level1_exit_count;
    context.events.push_back("Level1Machine::on_exit");
  }

  bool finish() {
    return context.finish_level1;
  }

  void on_finish() {
    context.events.push_back("Level1Machine::on_finish");
  }

  using initial_state = Level2StateRef;

  using transition_table = std::tuple<
      transition<
          Level2StateRef,
          DoneState,
          &Level1Machine::finish,
          &Level1Machine::on_finish
      >
  >;
};


// ============================================================
// Fixture
// ============================================================

class ThreeLevelStateMachineTest : public ::testing::Test {
 protected:
  void SetUp() override {
    context.reset();
  }
};


// ============================================================
// Start / nested initial state
// ============================================================

TEST_F(ThreeLevelStateMachineTest,
       StartsNestedInitialStateMachinesRecursively) {
  hfsm::state_machine<Level1Machine> sm;

  sm.start();

  EXPECT_EQ(
      sm.current_state(),
      Level1State::Level2);

  auto& level2 =
      sm.template get_state<Level2Machine>();

  EXPECT_EQ(
      level2.current_state(),
      Level2State::Level3);

  auto& level3 =
      level2.template get_state<Level3Machine>();

  EXPECT_EQ(
      level3.current_state(),
      Level3State::Idle);
}


// ============================================================
// on_entry
// ============================================================

TEST_F(ThreeLevelStateMachineTest,
       CallsOnEntryRecursivelyWhenStarted) {
  hfsm::state_machine<Level1Machine> sm;

  sm.start();

  EXPECT_EQ(context.level1_entry_count, 1);
  EXPECT_EQ(context.level2_entry_count, 1);
  EXPECT_EQ(context.level3_entry_count, 1);
  EXPECT_EQ(context.idle_entry_count, 1);

  EXPECT_EQ(context.level1_update_count, 0);
  EXPECT_EQ(context.level2_update_count, 0);
  EXPECT_EQ(context.level3_update_count, 0);
  EXPECT_EQ(context.idle_update_count, 0);

  EXPECT_EQ(context.level1_exit_count, 0);
  EXPECT_EQ(context.level2_exit_count, 0);
  EXPECT_EQ(context.level3_exit_count, 0);
  EXPECT_EQ(context.idle_exit_count, 0);
}


// ============================================================
// on_entry order
// ============================================================

TEST_F(ThreeLevelStateMachineTest,
       CallsOnEntryFromOuterToInner) {
  hfsm::state_machine<Level1Machine> sm;

  sm.start();

  const std::vector<std::string> expected = {
      "Level1Machine::on_entry",
      "Level2Machine::on_entry",
      "Level3Machine::on_entry",
      "Idle::on_entry"
  };

  EXPECT_EQ(context.events, expected);
}


// ============================================================
// on_update
// ============================================================

TEST_F(ThreeLevelStateMachineTest,
       CallsOnUpdateRecursively) {
  hfsm::state_machine<Level1Machine> sm;

  sm.start();

  context.events.clear();

  sm.step();

  // According to the current implementation:
  //
  // Level1Machine backend::step()
  //   -> Level2Machine::on_update()
  //        -> Level2 backend::step()
  //             -> Level3Machine::on_update()
  //                  -> Level3 backend::step()
  //                       -> Idle::on_update()
  //
  // Level1Machine::on_update() itself is not called by the
  // top-level sm.step().

  EXPECT_EQ(context.level1_update_count, 0);
  EXPECT_EQ(context.level2_update_count, 1);
  EXPECT_EQ(context.level3_update_count, 1);
  EXPECT_EQ(context.idle_update_count, 1);
}


// ============================================================
// on_update order
// ============================================================

TEST_F(ThreeLevelStateMachineTest,
       CallsOnUpdateFromOuterSubStateToInnerState) {
  hfsm::state_machine<Level1Machine> sm;

  sm.start();

  context.events.clear();

  sm.step();

  const std::vector<std::string> expected = {
      "Level2Machine::on_update",
      "Level3Machine::on_update",
      "Idle::on_update"
  };

  EXPECT_EQ(context.events, expected);
}


// ============================================================
// on_exit
// ============================================================

TEST_F(ThreeLevelStateMachineTest,
       CallsOnExitRecursivelyWhenLeavingNestedStateMachine) {
  hfsm::state_machine<Level1Machine> sm;

  sm.start();

  context.events.clear();

  context.finish_level1 = true;

  sm.step();

  EXPECT_EQ(
      sm.current_state(),
      Level1State::Done);

  EXPECT_EQ(context.idle_exit_count, 1);
  EXPECT_EQ(context.level3_exit_count, 1);
  EXPECT_EQ(context.level2_exit_count, 1);

  // Top-level Level1Machine itself has not been exited.
  EXPECT_EQ(context.level1_exit_count, 0);

  EXPECT_EQ(context.level1_done_entry_count, 1);
}


// ============================================================
// on_exit order
// ============================================================

TEST_F(ThreeLevelStateMachineTest,
       CallsOnExitFromInnerToOuter) {
  hfsm::state_machine<Level1Machine> sm;

  sm.start();

  context.events.clear();

  context.finish_level1 = true;

  sm.step();

  const std::vector<std::string> expected = {
      "Idle::on_exit",
      "Level3Machine::on_exit",
      "Level2Machine::on_exit",
      "Level1Machine::on_finish",
      "Level1Done::on_entry"
  };

  EXPECT_EQ(context.events, expected);
}


// ============================================================
// Complete lifecycle
// ============================================================

TEST_F(ThreeLevelStateMachineTest,
       ExecutesNestedEntryUpdateAndExitLifecycle) {
  hfsm::state_machine<Level1Machine> sm;

  // ----------------------------------------------------------
  // Entry
  // ----------------------------------------------------------

  sm.start();

  EXPECT_EQ(context.level1_entry_count, 1);
  EXPECT_EQ(context.level2_entry_count, 1);
  EXPECT_EQ(context.level3_entry_count, 1);
  EXPECT_EQ(context.idle_entry_count, 1);

  // ----------------------------------------------------------
  // Update
  // ----------------------------------------------------------

  sm.step();

  EXPECT_EQ(context.level2_update_count, 1);
  EXPECT_EQ(context.level3_update_count, 1);
  EXPECT_EQ(context.idle_update_count, 1);

  // ----------------------------------------------------------
  // Exit entire Level2 subtree
  // ----------------------------------------------------------

  context.finish_level1 = true;

  sm.step();

  EXPECT_EQ(context.idle_exit_count, 1);
  EXPECT_EQ(context.level3_exit_count, 1);
  EXPECT_EQ(context.level2_exit_count, 1);

  EXPECT_EQ(
      sm.current_state(),
      Level1State::Done);

  EXPECT_EQ(context.level1_done_entry_count, 1);
}