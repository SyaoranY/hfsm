#ifndef HFSM_STATE_MACHINE_H_
#define HFSM_STATE_MACHINE_H_

#include <array>
#include <hfsm/state_machine_def.h>
#include <hfsm/state_machine_mpl.h>
#include <stdexcept>

namespace hfsm {

/**
 * @brief Runtime backend for a hierarchical finite state machine.
 *
 * The state machine has a one-shot lifetime. start() initializes the machine
 * and may only be called once. After startup, step() advances the active
 * hierarchy one update at a time.
 *
 * Nested state machines are instantiated automatically and managed recursively.
 *
 * @tparam StateMachineDef A type derived from state_machine_def.
 */
template<typename StateMachineDef>
class state_machine {
  static_assert(
      detail::is_derived_from_state_machine_def<StateMachineDef>::value,
      "template parameter must be derived from state_machine_def"
  );

 public:
  using state_flag = state_machine_backend_tag;
  using enum_type = typename StateMachineDef::enum_type;
  using guard_type = typename StateMachineDef::guard_type;
  using action_type = typename StateMachineDef::action_type;
  using initial_state = typename StateMachineDef::initial_state;
  using transition_table = typename StateMachineDef::transition_table;
  using transition_entry_t = typename StateMachineDef::transition_entry_t;
  static constexpr std::size_t tt_size = hfsm::mpl::mp_size<transition_table>::value;
  using transition_entries_t = std::array<transition_entry_t, tt_size>;

  /**
   * @brief Starts the state machine.
   *
   * Sets the current state to the configured initial state, invokes the
   * machine-level on_entry() callback, enters the initial state, and resolves
   * any pseudo states encountered during startup.
   *
   * start() may only be called once during the lifetime of the machine.
   *
   * @throws std::logic_error If the state machine has already been started.
   */
  void start() {
    if (is_started_) {
      throw std::logic_error("start() cannot be called twice");
    }
    on_entry();
  }

  /**
   * @brief Advances the state machine by one update.
   *
   * The machine-level on_update() callback is invoked first. Transitions whose
   * source is the current state are then evaluated in transition-table order.
   * The first transition whose guard evaluates to true is taken.
   *
   * If a transition is taken at this machine level, the update is not
   * propagated to the active substate. Otherwise, the active substate receives
   * its on_update() callback. For a nested state machine, this recursively
   * updates its active hierarchy.
   *
   * Any pseudo states entered by a transition are resolved immediately.
   *
   * @throws std::logic_error If the state machine has not been started.
   * @throws std::logic_error If a pseudo state has no available outgoing
   *         transition or a pseudo-state transition cycle is detected.
   */
  void step() {
    if (!is_started_) {
      throw std::logic_error("step() cannot be called before start()");
    }
    state_machine_def_.on_update();
    transition_entry_t trans;
    if (find_available_transition(trans)) {
      do_transition(trans);
    } else {
      do_sub_state_update();
    }
  }

  /**
   * @brief Returns the currently active state.
   *
   * @pre The state machine must have been started with start().
   *
   * Calling this function before start() violates the API contract.
   *
   * @return Enumeration value of the currently active state.
   */
  enum_type current_state() const noexcept { return current_; }

  /**
   * @brief Checks whether the state machine has been started.
   *
   * Once started, a state machine remains started for the rest of its lifetime.
   *
   * @return true if start() has been called; otherwise false.
   */
  bool is_started() const noexcept { return is_started_; }

  /**
   * @brief Returns the stored instance of a state.
   *
   * For a nested state machine, the corresponding state_machine instance is
   * returned.
   *
   * The requested type must belong to this state machine.
   *
   * @tparam T State type or nested state-machine definition type.
   * @return Reference to the stored state instance.
   */
  template<typename T>
  auto& get_state() {
    using hfsm::mpl::mp_find_if;
    using hfsm::mpl::mp_size;
    constexpr size_t index = mp_find_if<sub_states_list_t, state_match<T>::template pred>::value;
    static_assert(index < mp_size<sub_states_list_t>::value, "template parameter T must be a valid state type");
    return std::get<index>(sub_states_);
  }

 private:
  template<typename SubStateMachineDef>
  friend class state_machine;

  // as a sub state machine
  void on_entry() {
    is_started_ = true;
    current_ = initial_state::enum_value;
    state_machine_def_.on_entry();
    do_sub_state_entry(current_);
    resolve_pseudo_states();
  }

  // as a sub state machine
  void on_exit() {
    do_sub_state_exit(current_);
    state_machine_def_.on_exit();
    current_ = initial_state::enum_value;
  }

  // as a sub state machine
  void on_update() { step(); }

  template<template<typename...> class L, typename... Trans>
  static constexpr auto construct_transition_entries(L<Trans...>*) -> transition_entries_t {
    return {Trans::make_transition_entry()...};
  }

  template<typename StateEntry, typename Tag = typename StateEntry::state_flag>
  struct sub_state_transform;

  template<typename StateEntry>
  struct sub_state_transform<StateEntry, normal_state_tag> {
    using type = StateEntry;
  };

  template<typename StateEntry>
  struct sub_state_transform<StateEntry, pseudo_state_tag> {
    using type = StateEntry;
  };

  template<typename StateEntry>
  struct sub_state_transform<StateEntry, state_machine_frontend_tag> {
    using state_type = typename StateEntry::state_type;
    using enum_type = typename StateEntry::enum_type;
    static constexpr enum_type enum_value = StateEntry::enum_value;
    using type = detail::state_entry<state_machine<state_type>, enum_type, enum_value>;
  };

  template<typename State>
  using sub_state_transform_t = typename sub_state_transform<State>::type;

  template<typename L>
  struct sub_states_list;

  template<template<typename...> class L, typename... Trans>
  struct sub_states_list<L<Trans...>> {
    using source_state_entry_list = hfsm::mpl::mp_list<typename Trans::source_state_entry_t...>;
    using target_state_entry_list = hfsm::mpl::mp_list<typename Trans::target_state_entry_t...>;
    using state_entry_list =
        hfsm::mpl::mp_set_union<hfsm::mpl::mp_list<initial_state>, source_state_entry_list, target_state_entry_list>;
    using type = hfsm::mpl::mp_apply<std::tuple, hfsm::mpl::mp_transform<sub_state_transform_t, state_entry_list>>;
  };

  using sub_states_list_t = typename sub_states_list<transition_table>::type;

  template<typename State>
  struct state_match {
    template<typename StateEntry>
    using pred = hfsm::mpl::mp_bool<
        std::is_same<typename StateEntry::state_type, State>::value ||
        std::is_same<typename StateEntry::state_type, state_machine<State>>::value>;
  };

  template<typename StateEntry>
  static constexpr bool is_pseudo_state_entry(StateEntry const&, pseudo_state_tag) {
    return true;
  }

  template<typename StateEntry, typename Tag>
  static constexpr bool is_pseudo_state_entry(StateEntry const&, Tag) {
    return false;
  }

  bool is_pseudo_state(enum_type state) const {
    bool result = false;
    hfsm::mpl::tuple_visit_if(
        sub_states_,
        [state](auto const& state_entry) { return state_entry.enum_value == state; },
        [&result](auto const& state_entry) {
          using state_entry_t = typename std::decay<decltype(state_entry)>::type;
          result = is_pseudo_state_entry(state_entry, typename state_entry_t::state_flag{});
        }
    );
    return result;
  }

  bool find_available_transition(transition_entry_t& trans) {
    for (auto const& trans_entry : transitions_) {
      if (trans_entry.source == current_ && (state_machine_def_.*trans_entry.guard)()) {
        trans = trans_entry;
        return true;
      }
    }
    return false;
  }

  void do_transition(transition_entry_t const& trans) {
    do_transition_once(trans);
    resolve_pseudo_states();
  }

  void do_transition_once(transition_entry_t const& trans) {
    do_sub_state_exit(trans.source);
    (state_machine_def_.*trans.action)();
    current_ = trans.target;
    do_sub_state_entry(trans.target);
  }

  void resolve_pseudo_states() {
    std::size_t transition_count = 0;

    while (is_pseudo_state(current_)) {
      if (++transition_count > tt_size) {
        throw std::logic_error("pseudo state transition cycle detected");
      }
      transition_entry_t trans;
      if (!find_available_transition(trans)) {
        throw std::logic_error("pseudo state has no available outgoing transition");
      }
      do_transition_once(trans);
    }
  }

  void do_sub_state_exit(enum_type const source) {
    hfsm::mpl::tuple_visit_if(
        sub_states_,
        [source](auto const& state_entry) { return state_entry.enum_value == source; },
        [](auto& state_entry) { state_entry.on_exit(); }
    );
  }

  void do_sub_state_entry(enum_type const target) {
    hfsm::mpl::tuple_visit_if(
        sub_states_,
        [target](auto const& state_entry) { return state_entry.enum_value == target; },
        [](auto& state_entry) { state_entry.on_entry(); }
    );
  }

  void do_sub_state_update() {
    hfsm::mpl::tuple_visit_if(
        sub_states_,
        [current = current_](auto const& state_entry) { return state_entry.enum_value == current; },
        [](auto& state_entry) { state_entry.on_update(); }
    );
  }

  bool is_started_ = false;
  enum_type current_;
  StateMachineDef state_machine_def_;
  sub_states_list_t sub_states_;
  transition_entries_t transitions_ = construct_transition_entries(static_cast<transition_table*>(nullptr));
};

} // namespace hfsm

#endif /* HFSM_STATE_MACHINE_H_ */
