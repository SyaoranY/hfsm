#include <gtest/gtest.h>
#include <hfsm/state_machine.h>

//
// ============================================================
// 1. Initial state is a pseudo state
// ============================================================
//

struct InitialPseudoContext {
  void reset() {
    go_idle = false;

    entry_pseudo_entry_count = 0;
    entry_pseudo_exit_count = 0;
    idle_entry_count = 0;
  }

  bool go_idle{false};

  int entry_pseudo_entry_count{0};
  int entry_pseudo_exit_count{0};
  int idle_entry_count{0};
};

InitialPseudoContext initial_pseudo_context;

enum class InitialPseudoState { Entry, Idle };

struct EntryPseudo : hfsm::pseudo_state<EntryPseudo> {
  void on_entry() { ++initial_pseudo_context.entry_pseudo_entry_count; }

  void on_exit() { ++initial_pseudo_context.entry_pseudo_exit_count; }
};

struct InitialIdle : hfsm::state<InitialIdle> {
  void on_entry() { ++initial_pseudo_context.idle_entry_count; }
};

struct InitialPseudoMachine : hfsm::state_machine_def<InitialPseudoMachine, InitialPseudoState> {
  using EntryState = state_entry<EntryPseudo, InitialPseudoState::Entry>;

  using IdleState = state_entry<InitialIdle, InitialPseudoState::Idle>;

  bool should_go_idle() { return initial_pseudo_context.go_idle; }

  void enter_idle() {}

  using initial_state = EntryState;

  using transition_table = hfsm::mpl::mp_list<
      transition<EntryState, IdleState, &InitialPseudoMachine::should_go_idle, &InitialPseudoMachine::enter_idle>>;
};

class InitialPseudoStateTest : public ::testing::Test {
 protected:
  void SetUp() override { initial_pseudo_context.reset(); }
};

TEST_F(InitialPseudoStateTest, ResolvesInitialPseudoStateDuringStart) {
  hfsm::state_machine<InitialPseudoMachine> sm;

  initial_pseudo_context.go_idle = true;

  sm.start();

  EXPECT_EQ(sm.current_state(), InitialPseudoState::Idle);

  EXPECT_EQ(initial_pseudo_context.entry_pseudo_entry_count, 1);

  EXPECT_EQ(initial_pseudo_context.entry_pseudo_exit_count, 1);

  EXPECT_EQ(initial_pseudo_context.idle_entry_count, 1);
}

TEST_F(InitialPseudoStateTest, DoesNotRemainInPseudoStateAfterStart) {
  hfsm::state_machine<InitialPseudoMachine> sm;

  initial_pseudo_context.go_idle = true;

  sm.start();

  EXPECT_NE(sm.current_state(), InitialPseudoState::Entry);

  EXPECT_EQ(sm.current_state(), InitialPseudoState::Idle);
}

TEST_F(InitialPseudoStateTest, ThrowsWhenInitialPseudoStateHasNoAvailableTransition) {
  hfsm::state_machine<InitialPseudoMachine> sm;

  initial_pseudo_context.go_idle = false;

  EXPECT_THROW(sm.start(), std::logic_error);
}

//
// ============================================================
// 2. Pseudo-state cycle
// ============================================================
//

struct PseudoCycleContext {
  void reset() {
    enter_cycle = false;

    p1_guard = false;
    p2_guard = false;

    p1_entry_count = 0;
    p1_exit_count = 0;
    p2_entry_count = 0;
    p2_exit_count = 0;
  }

  bool enter_cycle{false};

  bool p1_guard{false};
  bool p2_guard{false};

  int p1_entry_count{0};
  int p1_exit_count{0};

  int p2_entry_count{0};
  int p2_exit_count{0};
};

PseudoCycleContext pseudo_cycle_context;

enum class PseudoCycleState { Idle, Pseudo1, Pseudo2 };

struct CycleIdle : hfsm::state<CycleIdle> {};

struct Pseudo1 : hfsm::pseudo_state<Pseudo1> {
  void on_entry() { ++pseudo_cycle_context.p1_entry_count; }

  void on_exit() { ++pseudo_cycle_context.p1_exit_count; }
};

struct Pseudo2 : hfsm::pseudo_state<Pseudo2> {
  void on_entry() { ++pseudo_cycle_context.p2_entry_count; }

  void on_exit() { ++pseudo_cycle_context.p2_exit_count; }
};

struct PseudoCycleMachine : hfsm::state_machine_def<PseudoCycleMachine, PseudoCycleState> {
  using IdleState = state_entry<CycleIdle, PseudoCycleState::Idle>;

  using Pseudo1State = state_entry<Pseudo1, PseudoCycleState::Pseudo1>;

  using Pseudo2State = state_entry<Pseudo2, PseudoCycleState::Pseudo2>;

  bool should_enter_cycle() { return pseudo_cycle_context.enter_cycle; }

  bool pseudo1_to_pseudo2() { return pseudo_cycle_context.p1_guard; }

  bool pseudo2_to_pseudo1() { return pseudo_cycle_context.p2_guard; }

  void enter_pseudo1() {}

  void enter_pseudo2() {}

  void return_to_pseudo1() {}

  using initial_state = IdleState;

  using transition_table = hfsm::mpl::mp_list<
      transition<IdleState, Pseudo1State, &PseudoCycleMachine::should_enter_cycle, &PseudoCycleMachine::enter_pseudo1>,

      transition<
          Pseudo1State,
          Pseudo2State,
          &PseudoCycleMachine::pseudo1_to_pseudo2,
          &PseudoCycleMachine::enter_pseudo2>,

      transition<
          Pseudo2State,
          Pseudo1State,
          &PseudoCycleMachine::pseudo2_to_pseudo1,
          &PseudoCycleMachine::return_to_pseudo1>>;
};

class PseudoStateCycleTest : public ::testing::Test {
 protected:
  void SetUp() override { pseudo_cycle_context.reset(); }
};

TEST_F(PseudoStateCycleTest, ThrowsWhenPseudoStatesFormTransitionCycle) {
  hfsm::state_machine<PseudoCycleMachine> sm;

  sm.start();

  pseudo_cycle_context.enter_cycle = true;
  pseudo_cycle_context.p1_guard = true;
  pseudo_cycle_context.p2_guard = true;

  EXPECT_THROW(sm.step(), std::logic_error);
}

TEST_F(PseudoStateCycleTest, EntersAndExitsPseudoStatesBeforeCycleIsDetected) {
  hfsm::state_machine<PseudoCycleMachine> sm;

  sm.start();

  pseudo_cycle_context.enter_cycle = true;
  pseudo_cycle_context.p1_guard = true;
  pseudo_cycle_context.p2_guard = true;

  EXPECT_THROW(sm.step(), std::logic_error);

  EXPECT_GT(pseudo_cycle_context.p1_entry_count, 0);

  EXPECT_GT(pseudo_cycle_context.p1_exit_count, 0);

  EXPECT_GT(pseudo_cycle_context.p2_entry_count, 0);

  EXPECT_GT(pseudo_cycle_context.p2_exit_count, 0);
}