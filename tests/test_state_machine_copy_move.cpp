#include <gtest/gtest.h>
#include <hfsm/state_machine.h>
#include <hfsm/state_machine_def.h>
#include <tuple>
#include <type_traits>
#include <utility>

// ============================================================================
// Snapshot test machine
// ============================================================================

struct SnapshotContext {
  void reset() {
    enter_child = false;
    leave_child = false;
    activate_child = false;
    deactivate_child = false;

    reset_callbacks();
  }

  void reset_callbacks() {
    entry_count = 0;
    update_count = 0;
    exit_count = 0;
  }

  bool enter_child{false};
  bool leave_child{false};
  bool activate_child{false};
  bool deactivate_child{false};

  int entry_count{0};
  int update_count{0};
  int exit_count{0};
};

SnapshotContext snapshot_context;

// ----------------------------------------------------------------------------
// Child machine
// ----------------------------------------------------------------------------

enum class SnapshotChildState {
  Idle,
  Active,
};

struct SnapshotChildIdle : hfsm::state<SnapshotChildIdle> {
  void on_entry() { ++snapshot_context.entry_count; }
  void on_update() { ++snapshot_context.update_count; }
  void on_exit() { ++snapshot_context.exit_count; }

  int value{0};
};

struct SnapshotChildActive : hfsm::state<SnapshotChildActive> {
  void on_entry() { ++snapshot_context.entry_count; }
  void on_update() { ++snapshot_context.update_count; }
  void on_exit() { ++snapshot_context.exit_count; }

  int value{0};
};

struct SnapshotChildMachine : hfsm::state_machine_def<SnapshotChildMachine, SnapshotChildState> {
  using IdleState = state_entry<SnapshotChildIdle, SnapshotChildState::Idle>;

  using ActiveState = state_entry<SnapshotChildActive, SnapshotChildState::Active>;

  void on_entry() { ++snapshot_context.entry_count; }
  void on_update() { ++snapshot_context.update_count; }
  void on_exit() { ++snapshot_context.exit_count; }

  bool should_activate() { return snapshot_context.activate_child; }

  bool should_deactivate() { return snapshot_context.deactivate_child; }

  void activate() {}
  void deactivate() {}

  using initial_state = IdleState;

  using transition_table = std::tuple<
      transition<IdleState, ActiveState, &SnapshotChildMachine::should_activate, &SnapshotChildMachine::activate>,
      transition<ActiveState, IdleState, &SnapshotChildMachine::should_deactivate, &SnapshotChildMachine::deactivate>>;
};

// ----------------------------------------------------------------------------
// Root machine
// ----------------------------------------------------------------------------

enum class SnapshotRootState {
  Idle,
  Child,
};

struct SnapshotRootIdle : hfsm::state<SnapshotRootIdle> {
  void on_entry() { ++snapshot_context.entry_count; }
  void on_update() { ++snapshot_context.update_count; }
  void on_exit() { ++snapshot_context.exit_count; }

  int value{0};
};

struct SnapshotRootMachine : hfsm::state_machine_def<SnapshotRootMachine, SnapshotRootState> {
  using IdleState = state_entry<SnapshotRootIdle, SnapshotRootState::Idle>;

  using ChildState = state_entry<SnapshotChildMachine, SnapshotRootState::Child>;

  void on_entry() { ++snapshot_context.entry_count; }
  void on_update() { ++snapshot_context.update_count; }
  void on_exit() { ++snapshot_context.exit_count; }

  bool should_enter_child() { return snapshot_context.enter_child; }

  bool should_leave_child() { return snapshot_context.leave_child; }

  void enter_child() {}
  void leave_child() {}

  using initial_state = IdleState;

  using transition_table = std::tuple<
      transition<IdleState, ChildState, &SnapshotRootMachine::should_enter_child, &SnapshotRootMachine::enter_child>,
      transition<ChildState, IdleState, &SnapshotRootMachine::should_leave_child, &SnapshotRootMachine::leave_child>>;
};

using SnapshotMachine = hfsm::state_machine<SnapshotRootMachine>;

// These assertions lock down the intended public object semantics.
static_assert(std::is_default_constructible<SnapshotMachine>::value, "state_machine should be default constructible");

static_assert(std::is_copy_constructible<SnapshotMachine>::value, "state_machine should be copy constructible");

static_assert(std::is_copy_assignable<SnapshotMachine>::value, "state_machine should be copy assignable");

static_assert(std::is_move_constructible<SnapshotMachine>::value, "state_machine should be move constructible");

static_assert(std::is_move_assignable<SnapshotMachine>::value, "state_machine should be move assignable");

// ============================================================================
// Machine-definition-data test machine
// ============================================================================
//
// This machine is used to verify that StateMachineDef itself is part of the
// runtime snapshot.
//
// After one step, update_count == 1 and the machine is still Waiting.
// If that value is copied/moved correctly, one additional step on the
// destination increments it to 2 and immediately transitions to Done.
// ============================================================================

enum class DefinitionState {
  Waiting,
  Done,
};

struct Waiting : hfsm::state<Waiting> {};
struct Done : hfsm::state<Done> {};

struct DefinitionMachine : hfsm::state_machine_def<DefinitionMachine, DefinitionState> {
  using WaitingState = state_entry<Waiting, DefinitionState::Waiting>;

  using DoneState = state_entry<Done, DefinitionState::Done>;

  void on_update() { ++update_count; }

  bool ready() { return update_count >= 2; }

  void finish() {}

  using initial_state = WaitingState;

  using transition_table =
      std::tuple<transition<WaitingState, DoneState, &DefinitionMachine::ready, &DefinitionMachine::finish>>;

  int update_count{0};
};

using DefinitionSnapshotMachine = hfsm::state_machine<DefinitionMachine>;

// ============================================================================
// Fixture
// ============================================================================

class StateMachineCopyMoveTest : public ::testing::Test {
 protected:
  void SetUp() override { snapshot_context.reset(); }

  void prepare_active_snapshot(SnapshotMachine& sm) {
    sm.start();

    // Store data in the root state. This state becomes inactive later, so
    // copying it verifies that inactive states are also part of the snapshot.
    sm.get_state<SnapshotRootIdle>().value = 11;

    // Root: Idle -> Child
    snapshot_context.enter_child = true;
    sm.step();
    snapshot_context.enter_child = false;

    auto& child = sm.get_state<SnapshotChildMachine>();

    // Store data in the child's initial state before leaving it.
    child.get_state<SnapshotChildIdle>().value = 22;

    // Child: Idle -> Active
    snapshot_context.activate_child = true;
    sm.step();
    snapshot_context.activate_child = false;

    child.get_state<SnapshotChildActive>().value = 33;
  }

  void expect_active_snapshot(SnapshotMachine& sm) {
    EXPECT_TRUE(sm.is_started());
    EXPECT_EQ(sm.current_state(), SnapshotRootState::Child);

    EXPECT_EQ(sm.get_state<SnapshotRootIdle>().value, 11);

    auto& child = sm.get_state<SnapshotChildMachine>();

    EXPECT_TRUE(child.is_started());
    EXPECT_EQ(child.current_state(), SnapshotChildState::Active);

    EXPECT_EQ(child.get_state<SnapshotChildIdle>().value, 22);
    EXPECT_EQ(child.get_state<SnapshotChildActive>().value, 33);
  }
};

// ============================================================================
// Copy construction
// ============================================================================

TEST_F(StateMachineCopyMoveTest, CopiesUnstartedMachine) {
  SnapshotMachine source;

  source.get_state<SnapshotRootIdle>().value = 42;

  SnapshotMachine copy(source);

  EXPECT_FALSE(source.is_started());
  EXPECT_FALSE(copy.is_started());

  EXPECT_EQ(source.get_state<SnapshotRootIdle>().value, 42);
  EXPECT_EQ(copy.get_state<SnapshotRootIdle>().value, 42);
}

TEST_F(StateMachineCopyMoveTest, CopyConstructorCopiesCompleteRuntimeSnapshot) {
  SnapshotMachine source;
  prepare_active_snapshot(source);

  snapshot_context.reset_callbacks();

  SnapshotMachine copy(source);

  // Copy construction is a C++ object operation, not an FSM lifecycle event.
  EXPECT_EQ(snapshot_context.entry_count, 0);
  EXPECT_EQ(snapshot_context.update_count, 0);
  EXPECT_EQ(snapshot_context.exit_count, 0);

  expect_active_snapshot(source);
  expect_active_snapshot(copy);
}

TEST_F(StateMachineCopyMoveTest, CopiedMachinesRunIndependently) {
  SnapshotMachine source;
  prepare_active_snapshot(source);

  SnapshotMachine copy(source);

  auto& source_child = source.get_state<SnapshotChildMachine>();
  auto& copy_child = copy.get_state<SnapshotChildMachine>();

  copy_child.get_state<SnapshotChildActive>().value = 100;

  EXPECT_EQ(source_child.get_state<SnapshotChildActive>().value, 33);

  EXPECT_EQ(copy_child.get_state<SnapshotChildActive>().value, 100);

  // Advance only the copy.
  snapshot_context.deactivate_child = true;
  copy.step();
  snapshot_context.deactivate_child = false;

  EXPECT_EQ(source_child.current_state(), SnapshotChildState::Active);

  EXPECT_EQ(copy_child.current_state(), SnapshotChildState::Idle);
}

// ============================================================================
// Copy assignment
// ============================================================================

TEST_F(StateMachineCopyMoveTest, CopyAssignmentReplacesRuntimeSnapshot) {
  SnapshotMachine source;
  prepare_active_snapshot(source);

  SnapshotMachine target;
  target.start();

  target.get_state<SnapshotRootIdle>().value = 999;

  ASSERT_EQ(target.current_state(), SnapshotRootState::Idle);

  snapshot_context.reset_callbacks();

  target = source;

  // Assignment must not behave like an FSM transition.
  EXPECT_EQ(snapshot_context.entry_count, 0);
  EXPECT_EQ(snapshot_context.update_count, 0);
  EXPECT_EQ(snapshot_context.exit_count, 0);

  expect_active_snapshot(target);
  expect_active_snapshot(source);
}

TEST_F(StateMachineCopyMoveTest, CopyAssignmentProducesIndependentSnapshot) {
  SnapshotMachine source;
  prepare_active_snapshot(source);

  SnapshotMachine target;
  target.start();

  target = source;

  auto& source_child = source.get_state<SnapshotChildMachine>();
  auto& target_child = target.get_state<SnapshotChildMachine>();

  target_child.get_state<SnapshotChildActive>().value = 100;

  EXPECT_EQ(source_child.get_state<SnapshotChildActive>().value, 33);

  EXPECT_EQ(target_child.get_state<SnapshotChildActive>().value, 100);
}

// ============================================================================
// Move construction
// ============================================================================

TEST_F(StateMachineCopyMoveTest, MoveConstructorTransfersRuntimeSnapshot) {
  SnapshotMachine source;
  prepare_active_snapshot(source);

  snapshot_context.reset_callbacks();

  SnapshotMachine moved(std::move(source));

  // Move construction is also not an FSM lifecycle event.
  EXPECT_EQ(snapshot_context.entry_count, 0);
  EXPECT_EQ(snapshot_context.update_count, 0);
  EXPECT_EQ(snapshot_context.exit_count, 0);

  expect_active_snapshot(moved);

  // Do not inspect source here. Its post-move runtime state is intentionally
  // unspecified.
}

TEST_F(StateMachineCopyMoveTest, MoveConstructedMachineCanContinueRunning) {
  SnapshotMachine source;
  prepare_active_snapshot(source);

  SnapshotMachine moved(std::move(source));

  auto& child = moved.get_state<SnapshotChildMachine>();

  ASSERT_EQ(child.current_state(), SnapshotChildState::Active);

  snapshot_context.deactivate_child = true;
  moved.step();
  snapshot_context.deactivate_child = false;

  EXPECT_EQ(child.current_state(), SnapshotChildState::Idle);
}

// ============================================================================
// Move assignment
// ============================================================================

TEST_F(StateMachineCopyMoveTest, MoveAssignmentReplacesRuntimeSnapshot) {
  SnapshotMachine source;
  prepare_active_snapshot(source);

  SnapshotMachine target;
  target.start();

  target.get_state<SnapshotRootIdle>().value = 999;

  snapshot_context.reset_callbacks();

  target = std::move(source);

  // Replacing the target snapshot must not invoke FSM lifecycle callbacks.
  EXPECT_EQ(snapshot_context.entry_count, 0);
  EXPECT_EQ(snapshot_context.update_count, 0);
  EXPECT_EQ(snapshot_context.exit_count, 0);

  expect_active_snapshot(target);

  // Do not inspect source after the move.
}

TEST_F(StateMachineCopyMoveTest, MoveAssignedMachineCanContinueRunning) {
  SnapshotMachine source;
  prepare_active_snapshot(source);

  SnapshotMachine target;
  target.start();

  target = std::move(source);

  auto& child = target.get_state<SnapshotChildMachine>();

  ASSERT_EQ(child.current_state(), SnapshotChildState::Active);

  snapshot_context.deactivate_child = true;
  target.step();
  snapshot_context.deactivate_child = false;

  EXPECT_EQ(child.current_state(), SnapshotChildState::Idle);
}

// ============================================================================
// StateMachineDef snapshot
// ============================================================================

TEST_F(StateMachineCopyMoveTest, CopyConstructorCopiesMachineDefinitionData) {
  DefinitionSnapshotMachine source;

  source.start();

  // update_count becomes 1. Guard is still false.
  source.step();

  ASSERT_EQ(source.current_state(), DefinitionState::Waiting);

  DefinitionSnapshotMachine copy(source);

  // If update_count was copied as 1, this becomes 2 and transitions to Done.
  copy.step();

  EXPECT_EQ(copy.current_state(), DefinitionState::Done);

  // Source remains independent.
  EXPECT_EQ(source.current_state(), DefinitionState::Waiting);
}

TEST_F(StateMachineCopyMoveTest, CopyAssignmentCopiesMachineDefinitionData) {
  DefinitionSnapshotMachine source;
  DefinitionSnapshotMachine target;

  source.start();
  target.start();

  source.step();

  ASSERT_EQ(source.current_state(), DefinitionState::Waiting);

  target = source;

  target.step();

  EXPECT_EQ(target.current_state(), DefinitionState::Done);

  EXPECT_EQ(source.current_state(), DefinitionState::Waiting);
}

TEST_F(StateMachineCopyMoveTest, MoveConstructorMovesMachineDefinitionData) {
  DefinitionSnapshotMachine source;

  source.start();
  source.step();

  ASSERT_EQ(source.current_state(), DefinitionState::Waiting);

  DefinitionSnapshotMachine moved(std::move(source));

  moved.step();

  EXPECT_EQ(moved.current_state(), DefinitionState::Done);
}

TEST_F(StateMachineCopyMoveTest, MoveAssignmentMovesMachineDefinitionData) {
  DefinitionSnapshotMachine source;
  DefinitionSnapshotMachine target;

  source.start();
  target.start();

  source.step();

  ASSERT_EQ(source.current_state(), DefinitionState::Waiting);

  target = std::move(source);

  target.step();

  EXPECT_EQ(target.current_state(), DefinitionState::Done);
}