#include <gtest/gtest.h>
#include <hfsm/state_machine.h>
#include <hfsm/state_machine_def.h>

// context
struct VehicleContext {
  void reset() {
    power_on = false;
    engage_autonomous = false;
    disengage_autonomous = false;
    system_fault = false;
    reset_requested = false;

    autonomous_ready = false;
    vehicle_ahead = false;
    lane_change_requested = false;
    lane_change_completed = false;
    parking_requested = false;
    parking_completed = false;
  }

  bool power_on{false};
  bool engage_autonomous{false};
  bool disengage_autonomous{false};
  bool system_fault{false};
  bool reset_requested{false};

  bool autonomous_ready{false};
  bool vehicle_ahead{false};
  bool lane_change_requested{false};
  bool lane_change_completed{false};
  bool parking_requested{false};
  bool parking_completed{false};
};

VehicleContext context;

// sub state machine  Autonomous
enum class AutonomousState { Standby, Cruising, Following, LaneChanging, Parking };

struct Standby : hfsm::state<Standby> {};
struct Cruising : hfsm::state<Cruising> {};
struct Following : hfsm::state<Following> {};
struct LaneChanging : hfsm::state<LaneChanging> {};
struct Parking : hfsm::state<Parking> {};

struct Autonomous : hfsm::state_machine_def<Autonomous, AutonomousState> {
  using StandbyState = state_entry<Standby, AutonomousState::Standby>;
  using CruisingState = state_entry<Cruising, AutonomousState::Cruising>;
  using FollowingState = state_entry<Following, AutonomousState::Following>;
  using LaneChangingState = state_entry<LaneChanging, AutonomousState::LaneChanging>;
  using ParkingState = state_entry<Parking, AutonomousState::Parking>;

  bool can_start_cruising() { return context.autonomous_ready; }

  bool vehicle_ahead() { return context.vehicle_ahead; }

  bool road_clear() { return !context.vehicle_ahead; }

  bool lane_change_requested() { return context.lane_change_requested; }

  bool lane_change_completed() { return context.lane_change_completed; }

  bool parking_requested() { return context.parking_requested; }

  bool parking_completed() { return context.parking_completed; }

  void start_cruising() {}
  void start_following() {}
  void resume_cruising() {}
  void start_lane_change() {}
  void finish_lane_change() {}
  void start_parking() {}
  void finish_parking() {}

  using initial_state = StandbyState;

  using transition_table = std::tuple<
      transition<StandbyState, CruisingState, &Autonomous::can_start_cruising, &Autonomous::start_cruising>,
      transition<CruisingState, FollowingState, &Autonomous::vehicle_ahead, &Autonomous::start_following>,
      transition<FollowingState, CruisingState, &Autonomous::road_clear, &Autonomous::resume_cruising>,
      transition<CruisingState, LaneChangingState, &Autonomous::lane_change_requested, &Autonomous::start_lane_change>,
      transition<LaneChangingState, CruisingState, &Autonomous::lane_change_completed, &Autonomous::finish_lane_change>,
      transition<CruisingState, ParkingState, &Autonomous::parking_requested, &Autonomous::start_parking>,
      transition<FollowingState, ParkingState, &Autonomous::parking_requested, &Autonomous::start_parking>,
      transition<ParkingState, StandbyState, &Autonomous::parking_completed, &Autonomous::finish_parking>>;
};

// top state machine
enum class VehicleState { Off, Manual, Autonomous, Emergency };

struct Off : hfsm::state<Off> {};
struct Manual : hfsm::state<Manual> {};
struct Emergency : hfsm::state<Emergency> {};

struct Vehicle : hfsm::state_machine_def<Vehicle, VehicleState> {
  using OffState = state_entry<Off, VehicleState::Off>;
  using ManualState = state_entry<Manual, VehicleState::Manual>;
  using AutonomousStateRef = state_entry<Autonomous, VehicleState::Autonomous>;
  using EmergencyState = state_entry<Emergency, VehicleState::Emergency>;

  bool power_on() { return context.power_on; }
  bool engage_autonomous() { return context.engage_autonomous; }
  bool disengage_autonomous() { return context.disengage_autonomous; }
  bool system_fault() { return context.system_fault; }
  bool reset_requested() { return context.reset_requested; }

  void start_manual() {}
  void start_autonomous() {}
  void stop_autonomous() {}
  void enter_emergency() {}
  void reset_vehicle() {}

  using initial_state = OffState;

  using transition_table = std::tuple<
      transition<OffState, ManualState, &Vehicle::power_on, &Vehicle::start_manual>,
      transition<ManualState, AutonomousStateRef, &Vehicle::engage_autonomous, &Vehicle::start_autonomous>,
      transition<AutonomousStateRef, ManualState, &Vehicle::disengage_autonomous, &Vehicle::stop_autonomous>,
      transition<ManualState, EmergencyState, &Vehicle::system_fault, &Vehicle::enter_emergency>,
      transition<AutonomousStateRef, EmergencyState, &Vehicle::system_fault, &Vehicle::enter_emergency>,
      transition<EmergencyState, OffState, &Vehicle::reset_requested, &Vehicle::reset_vehicle>>;
};

class HierarchicalStateMachineTest : public ::testing::Test {
 protected:
  void SetUp() override { context.reset(); }
};

// ============================================================
// Initial state
// ============================================================

TEST_F(HierarchicalStateMachineTest, StartsFromOff) {
  hfsm::state_machine<Vehicle> sm;

  sm.start();

  EXPECT_EQ(sm.current_state(), VehicleState::Off);
}

// ============================================================
// Off -> Manual
// ============================================================

TEST_F(HierarchicalStateMachineTest, TransitionsFromOffToManual) {
  hfsm::state_machine<Vehicle> sm;
  sm.start();

  context.power_on = true;
  sm.step();

  EXPECT_EQ(sm.current_state(), VehicleState::Manual);
}

// ============================================================
// Manual -> Autonomous
// ============================================================

TEST_F(HierarchicalStateMachineTest, TransitionsFromManualToAutonomous) {
  hfsm::state_machine<Vehicle> sm;
  sm.start();

  context.power_on = true;
  sm.step();

  context.power_on = false;
  context.engage_autonomous = true;
  sm.step();

  EXPECT_EQ(sm.current_state(), VehicleState::Autonomous);
}

// ============================================================
// Entering Autonomous starts its initial state
// ============================================================

TEST_F(HierarchicalStateMachineTest, StartsAutonomousFromStandbyWhenEnteringAutonomous) {
  hfsm::state_machine<Vehicle> sm;
  sm.start();

  context.power_on = true;
  sm.step();

  context.power_on = false;
  context.engage_autonomous = true;
  sm.step();

  EXPECT_EQ(sm.current_state(), VehicleState::Autonomous);

  auto& autonomous = sm.get_state<Autonomous>();

  EXPECT_EQ(autonomous.current_state(), AutonomousState::Standby);
}

// ============================================================
// Standby -> Cruising
// ============================================================

TEST_F(HierarchicalStateMachineTest, AutonomousTransitionsFromStandbyToCruising) {
  hfsm::state_machine<Vehicle> sm;
  sm.start();

  context.power_on = true;
  sm.step();

  context.power_on = false;
  context.engage_autonomous = true;
  sm.step();

  context.engage_autonomous = false;
  context.autonomous_ready = true;
  sm.step();

  auto& autonomous = sm.get_state<Autonomous>();

  EXPECT_EQ(sm.current_state(), VehicleState::Autonomous);
  EXPECT_EQ(autonomous.current_state(), AutonomousState::Cruising);
}

// ============================================================
// Cruising -> Following
// ============================================================

TEST_F(HierarchicalStateMachineTest, AutonomousTransitionsFromCruisingToFollowing) {
  hfsm::state_machine<Vehicle> sm;
  sm.start();

  context.power_on = true;
  sm.step();

  context.power_on = false;
  context.engage_autonomous = true;
  sm.step();

  context.engage_autonomous = false;
  context.autonomous_ready = true;
  sm.step();

  context.autonomous_ready = false;
  context.vehicle_ahead = true;
  sm.step();

  auto& autonomous = sm.get_state<Autonomous>();

  EXPECT_EQ(autonomous.current_state(), AutonomousState::Following);
}

// ============================================================
// Following -> Cruising
// ============================================================

TEST_F(HierarchicalStateMachineTest, AutonomousTransitionsFromFollowingBackToCruising) {
  hfsm::state_machine<Vehicle> sm;
  sm.start();

  context.power_on = true;
  sm.step();

  context.power_on = false;
  context.engage_autonomous = true;
  sm.step();

  context.engage_autonomous = false;
  context.autonomous_ready = true;
  sm.step();

  context.autonomous_ready = false;
  context.vehicle_ahead = true;
  sm.step();

  context.vehicle_ahead = false;
  sm.step();

  auto& autonomous = sm.get_state<Autonomous>();

  EXPECT_EQ(autonomous.current_state(), AutonomousState::Cruising);
}

// ============================================================
// Cruising -> LaneChanging -> Cruising
// ============================================================

TEST_F(HierarchicalStateMachineTest, AutonomousPerformsLaneChange) {
  hfsm::state_machine<Vehicle> sm;
  sm.start();

  context.power_on = true;
  sm.step();

  context.power_on = false;
  context.engage_autonomous = true;
  sm.step();

  context.engage_autonomous = false;
  context.autonomous_ready = true;
  sm.step();

  auto& autonomous = sm.get_state<Autonomous>();

  EXPECT_EQ(autonomous.current_state(), AutonomousState::Cruising);

  context.autonomous_ready = false;
  context.lane_change_requested = true;
  sm.step();

  EXPECT_EQ(autonomous.current_state(), AutonomousState::LaneChanging);

  context.lane_change_requested = false;
  context.lane_change_completed = true;
  sm.step();

  EXPECT_EQ(autonomous.current_state(), AutonomousState::Cruising);
}

// ============================================================
// Cruising -> Parking
// ============================================================

TEST_F(HierarchicalStateMachineTest, AutonomousTransitionsFromCruisingToParking) {
  hfsm::state_machine<Vehicle> sm;
  sm.start();

  context.power_on = true;
  sm.step();

  context.power_on = false;
  context.engage_autonomous = true;
  sm.step();

  context.engage_autonomous = false;
  context.autonomous_ready = true;
  sm.step();

  context.autonomous_ready = false;
  context.parking_requested = true;
  sm.step();

  auto& autonomous = sm.get_state<Autonomous>();

  EXPECT_EQ(autonomous.current_state(), AutonomousState::Parking);
}

// ============================================================
// Following -> Parking
// ============================================================

TEST_F(HierarchicalStateMachineTest, AutonomousTransitionsFromFollowingToParking) {
  hfsm::state_machine<Vehicle> sm;
  sm.start();

  context.power_on = true;
  sm.step();

  context.power_on = false;
  context.engage_autonomous = true;
  sm.step();

  context.engage_autonomous = false;
  context.autonomous_ready = true;
  sm.step();

  context.autonomous_ready = false;
  context.vehicle_ahead = true;
  sm.step();

  context.parking_requested = true;
  sm.step();

  auto& autonomous = sm.get_state<Autonomous>();

  EXPECT_EQ(autonomous.current_state(), AutonomousState::Parking);
}

// ============================================================
// Parking -> Standby
// ============================================================

TEST_F(HierarchicalStateMachineTest, AutonomousReturnsToStandbyAfterParking) {
  hfsm::state_machine<Vehicle> sm;
  sm.start();

  context.power_on = true;
  sm.step();

  context.power_on = false;
  context.engage_autonomous = true;
  sm.step();

  context.engage_autonomous = false;
  context.autonomous_ready = true;
  sm.step();

  context.autonomous_ready = false;
  context.parking_requested = true;
  sm.step();

  context.parking_requested = false;
  context.parking_completed = true;
  sm.step();

  auto& autonomous = sm.get_state<Autonomous>();

  EXPECT_EQ(autonomous.current_state(), AutonomousState::Standby);
}

// ============================================================
// Autonomous -> Manual
// ============================================================

TEST_F(HierarchicalStateMachineTest, ExitsAutonomousAndReturnsToManual) {
  hfsm::state_machine<Vehicle> sm;
  sm.start();

  context.power_on = true;
  sm.step();

  context.power_on = false;
  context.engage_autonomous = true;
  sm.step();

  context.engage_autonomous = false;
  context.disengage_autonomous = true;
  sm.step();

  EXPECT_EQ(sm.current_state(), VehicleState::Manual);
}

// ============================================================
// Parent transition should work regardless of child state
// ============================================================

TEST_F(HierarchicalStateMachineTest, ExitsAutonomousWhileChildIsCruising) {
  hfsm::state_machine<Vehicle> sm;
  sm.start();

  // Off -> Manual
  context.power_on = true;
  sm.step();

  // Manual -> Autonomous
  context.power_on = false;
  context.engage_autonomous = true;
  sm.step();

  // Standby -> Cruising
  context.engage_autonomous = false;
  context.autonomous_ready = true;
  sm.step();

  auto& autonomous = sm.get_state<Autonomous>();

  EXPECT_EQ(autonomous.current_state(), AutonomousState::Cruising);

  // Autonomous -> Manual
  context.autonomous_ready = false;
  context.disengage_autonomous = true;
  sm.step();

  EXPECT_EQ(sm.current_state(), VehicleState::Manual);
}

// ============================================================
// Autonomous -> Emergency from nested state
// ============================================================

TEST_F(HierarchicalStateMachineTest, ParentTransitionToEmergencyOverridesNestedState) {
  hfsm::state_machine<Vehicle> sm;
  sm.start();

  // Off -> Manual
  context.power_on = true;
  sm.step();

  // Manual -> Autonomous
  context.power_on = false;
  context.engage_autonomous = true;
  sm.step();

  // Standby -> Cruising
  context.engage_autonomous = false;
  context.autonomous_ready = true;
  sm.step();

  // Cruising -> Following
  context.autonomous_ready = false;
  context.vehicle_ahead = true;
  sm.step();

  auto& autonomous = sm.get_state<Autonomous>();

  EXPECT_EQ(autonomous.current_state(), AutonomousState::Following);

  // Fault occurs while nested state is Following
  context.vehicle_ahead = false;
  context.system_fault = true;
  sm.step();

  EXPECT_EQ(sm.current_state(), VehicleState::Emergency);
}

// ============================================================
// Manual -> Emergency
// ============================================================

TEST_F(HierarchicalStateMachineTest, TransitionsFromManualToEmergency) {
  hfsm::state_machine<Vehicle> sm;
  sm.start();

  context.power_on = true;
  sm.step();

  context.power_on = false;
  context.system_fault = true;
  sm.step();

  EXPECT_EQ(sm.current_state(), VehicleState::Emergency);
}

// ============================================================
// Emergency -> Off
// ============================================================

TEST_F(HierarchicalStateMachineTest, ResetsFromEmergencyToOff) {
  hfsm::state_machine<Vehicle> sm;
  sm.start();

  context.power_on = true;
  sm.step();

  context.power_on = false;
  context.system_fault = true;
  sm.step();

  EXPECT_EQ(sm.current_state(), VehicleState::Emergency);

  context.system_fault = false;
  context.reset_requested = true;
  sm.step();

  EXPECT_EQ(sm.current_state(), VehicleState::Off);
}

// ============================================================
// Full hierarchical path
// ============================================================

TEST_F(HierarchicalStateMachineTest, ExecutesCompleteHierarchicalStateSequence) {
  hfsm::state_machine<Vehicle> sm;
  sm.start();

  auto& autonomous = sm.get_state<Autonomous>();

  // Off
  EXPECT_EQ(sm.current_state(), VehicleState::Off);

  // Off -> Manual
  context.power_on = true;
  sm.step();

  EXPECT_EQ(sm.current_state(), VehicleState::Manual);

  // Manual -> Autonomous
  context.power_on = false;
  context.engage_autonomous = true;
  sm.step();

  EXPECT_EQ(sm.current_state(), VehicleState::Autonomous);
  EXPECT_EQ(autonomous.current_state(), AutonomousState::Standby);

  // Standby -> Cruising
  context.engage_autonomous = false;
  context.autonomous_ready = true;
  sm.step();

  EXPECT_EQ(autonomous.current_state(), AutonomousState::Cruising);

  // Cruising -> Following
  context.autonomous_ready = false;
  context.vehicle_ahead = true;
  sm.step();

  EXPECT_EQ(autonomous.current_state(), AutonomousState::Following);

  // Following -> Cruising
  context.vehicle_ahead = false;
  sm.step();

  EXPECT_EQ(autonomous.current_state(), AutonomousState::Cruising);

  // Cruising -> LaneChanging
  context.lane_change_requested = true;
  sm.step();

  EXPECT_EQ(autonomous.current_state(), AutonomousState::LaneChanging);

  // LaneChanging -> Cruising
  context.lane_change_requested = false;
  context.lane_change_completed = true;
  sm.step();

  EXPECT_EQ(autonomous.current_state(), AutonomousState::Cruising);

  // Cruising -> Parking
  context.lane_change_completed = false;
  context.parking_requested = true;
  sm.step();

  EXPECT_EQ(autonomous.current_state(), AutonomousState::Parking);

  // Parking -> Standby
  context.parking_requested = false;
  context.parking_completed = true;
  sm.step();

  EXPECT_EQ(autonomous.current_state(), AutonomousState::Standby);

  // Autonomous -> Manual
  context.parking_completed = false;
  context.disengage_autonomous = true;
  sm.step();

  EXPECT_EQ(sm.current_state(), VehicleState::Manual);
}