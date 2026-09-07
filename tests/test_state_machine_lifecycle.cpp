#include <gtest/gtest.h>
#include <hfsm/state_machine.h>
#include <hfsm/state_machine_def.h>

struct LifecycleContext {
  void reset() {
    enter_child = false;
    leave_child = false;
    child_transition = false;

    enter_child_during_update = false;
    child_transition_during_update = false;

    root_entry_count = 0;
    root_update_count = 0;
    root_exit_count = 0;

    child_entry_count = 0;
    child_update_count = 0;
    child_exit_count = 0;
  }

  bool enter_child{false};
  bool leave_child{false};
  bool child_transition{false};

  bool enter_child_during_update{false};
  bool child_transition_during_update{false};

  int root_entry_count{0};
  int root_update_count{0};
  int root_exit_count{0};

  int child_entry_count{0};
  int child_update_count{0};
  int child_exit_count{0};
};

LifecycleContext lifecycle_context;

// -----------------------------------------------------------------------------
// Child state machine
// -----------------------------------------------------------------------------

enum class ChildState {
  Idle,
  Active,
};

struct ChildIdle : hfsm::state<ChildIdle> {};
struct ChildActive : hfsm::state<ChildActive> {};

struct ChildMachine : hfsm::state_machine_def<ChildMachine, ChildState> {
  using IdleState = state_entry<ChildIdle, ChildState::Idle>;
  using ActiveState = state_entry<ChildActive, ChildState::Active>;

  void on_entry() {
    ++lifecycle_context.child_entry_count;
  }

  void on_update() {
    ++lifecycle_context.child_update_count;

    if (lifecycle_context.child_transition_during_update) {
      lifecycle_context.child_transition = true;
    }
  }

  void on_exit() {
    ++lifecycle_context.child_exit_count;
  }

  bool should_activate() {
    return lifecycle_context.child_transition;
  }

  void activate() {}

  using initial_state = IdleState;

  using transition_table = std::tuple<
      transition<
          IdleState,
          ActiveState,
          &ChildMachine::should_activate,
          &ChildMachine::activate>>;
};

// -----------------------------------------------------------------------------
// Root state machine
// -----------------------------------------------------------------------------

enum class RootState {
  Idle,
  Child,
};

struct RootIdle : hfsm::state<RootIdle> {};

struct RootMachine : hfsm::state_machine_def<RootMachine, RootState> {
  using IdleState = state_entry<RootIdle, RootState::Idle>;
  using ChildStateRef = state_entry<ChildMachine, RootState::Child>;

  void on_entry() {
    ++lifecycle_context.root_entry_count;
  }

  void on_update() {
    ++lifecycle_context.root_update_count;

    if (lifecycle_context.enter_child_during_update) {
      lifecycle_context.enter_child = true;
    }
  }

  void on_exit() {
    ++lifecycle_context.root_exit_count;
  }

  bool should_enter_child() {
    return lifecycle_context.enter_child;
  }

  bool should_leave_child() {
    return lifecycle_context.leave_child;
  }

  void enter_child() {}
  void leave_child() {}

  using initial_state = IdleState;

  using transition_table = std::tuple<
      transition<
          IdleState,
          ChildStateRef,
          &RootMachine::should_enter_child,
          &RootMachine::enter_child>,
      transition<
          ChildStateRef,
          IdleState,
          &RootMachine::should_leave_child,
          &RootMachine::leave_child>>;
};

class StateMachineLifecycleTest : public ::testing::Test {
 protected:
  void SetUp() override {
    lifecycle_context.reset();
  }
};

TEST_F(StateMachineLifecycleTest, StartCallsRootMachineOnEntry) {
  hfsm::state_machine<RootMachine> sm;

  sm.start();

  EXPECT_EQ(lifecycle_context.root_entry_count, 1);
  EXPECT_EQ(lifecycle_context.root_update_count, 0);
  EXPECT_EQ(lifecycle_context.root_exit_count, 0);
}

TEST_F(StateMachineLifecycleTest, StepCallsRootMachineOnUpdate) {
  hfsm::state_machine<RootMachine> sm;
  sm.start();

  sm.step();
  sm.step();
  sm.step();

  EXPECT_EQ(lifecycle_context.root_update_count, 3);
}

TEST_F(StateMachineLifecycleTest, RootOnUpdateIsCalledWhenTransitionOccurs) {
  hfsm::state_machine<RootMachine> sm;
  sm.start();

  lifecycle_context.enter_child = true;
  sm.step();

  EXPECT_EQ(lifecycle_context.root_update_count, 1);
  EXPECT_EQ(sm.current_state(), RootState::Child);
}

TEST_F(StateMachineLifecycleTest, RootOnUpdateRunsBeforeTransitionGuards) {
  hfsm::state_machine<RootMachine> sm;
  sm.start();

  lifecycle_context.enter_child_during_update = true;

  sm.step();

  EXPECT_EQ(lifecycle_context.root_update_count, 1);
  EXPECT_EQ(sm.current_state(), RootState::Child);
}

TEST_F(StateMachineLifecycleTest, EnteringChildCallsChildMachineOnEntry) {
  hfsm::state_machine<RootMachine> sm;
  sm.start();

  lifecycle_context.enter_child = true;
  sm.step();

  EXPECT_EQ(sm.current_state(), RootState::Child);
  EXPECT_EQ(lifecycle_context.child_entry_count, 1);
  EXPECT_EQ(lifecycle_context.child_update_count, 0);
  EXPECT_EQ(lifecycle_context.child_exit_count, 0);
}

TEST_F(StateMachineLifecycleTest, UpdatingChildCallsChildMachineOnUpdate) {
  hfsm::state_machine<RootMachine> sm;
  sm.start();

  lifecycle_context.enter_child = true;
  sm.step();

  lifecycle_context.enter_child = false;

  sm.step();

  EXPECT_EQ(lifecycle_context.child_update_count, 1);
}

TEST_F(StateMachineLifecycleTest, ChildOnUpdateRunsBeforeTransitionGuards) {
  hfsm::state_machine<RootMachine> sm;
  sm.start();

  lifecycle_context.enter_child = true;
  sm.step();

  lifecycle_context.enter_child = false;
  lifecycle_context.child_transition_during_update = true;

  sm.step();

  auto& child = sm.get_state<ChildMachine>();

  EXPECT_EQ(lifecycle_context.child_update_count, 1);
  EXPECT_EQ(child.current_state(), ChildState::Active);
}

TEST_F(StateMachineLifecycleTest, ExitingChildCallsChildMachineOnExit) {
  hfsm::state_machine<RootMachine> sm;
  sm.start();

  lifecycle_context.enter_child = true;
  sm.step();

  lifecycle_context.enter_child = false;
  lifecycle_context.leave_child = true;

  sm.step();

  EXPECT_EQ(sm.current_state(), RootState::Idle);
  EXPECT_EQ(lifecycle_context.child_exit_count, 1);
}

TEST_F(StateMachineLifecycleTest, DestroyingRootDoesNotCallOnExit) {
  {
    hfsm::state_machine<RootMachine> sm;
    sm.start();
  }

  EXPECT_EQ(lifecycle_context.root_exit_count, 0);
}