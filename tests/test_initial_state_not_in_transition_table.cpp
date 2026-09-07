#include <gtest/gtest.h>
#include <hfsm/state_machine.h>
#include <tuple>

enum class IsolatedInitialState {
  Idle,
  Running,
  Stopped,
};

struct IsolatedIdle : hfsm::state<IsolatedIdle> {
  void on_entry() { ++entry_count; }
  void on_update() { ++update_count; }

  static int entry_count;
  static int update_count;
};

int IsolatedIdle::entry_count = 0;
int IsolatedIdle::update_count = 0;

struct IsolatedRunning : hfsm::state<IsolatedRunning> {};
struct IsolatedStopped : hfsm::state<IsolatedStopped> {};

struct IsolatedInitialMachine : hfsm::state_machine_def<IsolatedInitialMachine, IsolatedInitialState> {
  using IdleState = state_entry<IsolatedIdle, IsolatedInitialState::Idle>;
  using RunningState = state_entry<IsolatedRunning, IsolatedInitialState::Running>;
  using StoppedState = state_entry<IsolatedStopped, IsolatedInitialState::Stopped>;

  bool should_stop() { return false; }

  void do_stop() {}

  using initial_state = IdleState;

  // Idle does not appear anywhere in the transition table.
  using transition_table = std::tuple<
      transition<RunningState, StoppedState, &IsolatedInitialMachine::should_stop, &IsolatedInitialMachine::do_stop>>;
};

TEST(StateMachineTest, InitialStateDoesNotNeedToAppearInTransitionTable) {
  IsolatedIdle::entry_count = 0;
  IsolatedIdle::update_count = 0;

  hfsm::state_machine<IsolatedInitialMachine> sm;

  sm.start();

  EXPECT_EQ(sm.current_state(), IsolatedInitialState::Idle);
  EXPECT_EQ(IsolatedIdle::entry_count, 1);

  sm.step();

  EXPECT_EQ(sm.current_state(), IsolatedInitialState::Idle);
  EXPECT_EQ(IsolatedIdle::update_count, 1);
}