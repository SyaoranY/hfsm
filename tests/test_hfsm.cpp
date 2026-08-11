#include <tuple>

struct Autonomous : public hfsm::state_machine_def<Autonomous> {
  struct StandBy : public hfsm::state<StandBy> {
    void on_entry() { }
    void on_update() { }
    void on_exit() { }
  };

  struct Emergency : public hfsm::state<Emergency> {
    void on_entry() { }
    void on_update() { }
    void on_exit() { }
  };

  using initial_state = StandBy;

  using transition_table = std::tuple<


};


enum class VehicleState {
  Off,
  Manual,
  Autonomous
};

struct Vehicle : public hfsm::state_machine_def<Vehicle> {
  struct OFF : public hfsm::state<Off> {
    void on_entry() { }
    void on_update() { }
    void on_exit() { }
  };

  struct Manual : public hfsm::state<Manual> {
    void on_entry() { }
    void on_update() { }
    void on_exit() { }
  };

  bool check_off_to_manual() { return true; }
  void off_to_manual() { }

  bool check_manual_to_autonomous() { return true; }
  void manual_to_autonomous() { }

  bool check_manual_to_off() { return true; }
  void manual_to_off() { }

  bool check_autonomous_to_off() { return true; }
  void autonomous_to_off() { }

  using initial_state = OFF;
  using transition_table = std::tuple<
    // SourceState,  DestState, Guard, Action,
    transition<OFF, Manual, &Vehicle::check_off_to_manual, &Vehicle::off_to_manual>,
    transition<Manual, Autonomous, &Vehicle::check_manual_to_autonomous,  &Vehicle::manual_to_autonomous>,
    transition<Manual, OFF, &Vehicle::check_manual_to_off, &Vehicle::manual_to_off>,
    transition<Autonomous, OFF, &Vehicle::check_autonomous_to_off, &Vehicle::autonomous_to_off>
  >;
};