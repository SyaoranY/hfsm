#include <gtest/gtest.h>
#include <hfsm/state_machine.h>
#include <tuple>

enum class SingleState {
  Idle,
};

struct Idle : hfsm::state<Idle> {
  void on_entry() { ++entry_count; }
  void on_update() { ++update_count; }

  static int entry_count;
  static int update_count;
};

int Idle::entry_count = 0;
int Idle::update_count = 0;

struct SingleStateMachine : hfsm::state_machine_def<SingleStateMachine, SingleState> {
  using IdleState = state_entry<Idle, SingleState::Idle>;

  using initial_state = IdleState;
  using transition_table = std::tuple<>;
};

TEST(StateMachineTest, SupportsEmptyTransitionTable) {
  Idle::entry_count = 0;
  Idle::update_count = 0;

  hfsm::state_machine<SingleStateMachine> sm;

  sm.start();

  EXPECT_EQ(sm.current_state(), SingleState::Idle);
  EXPECT_EQ(Idle::entry_count, 1);

  sm.step();

  EXPECT_EQ(sm.current_state(), SingleState::Idle);
  EXPECT_EQ(Idle::update_count, 1);
}