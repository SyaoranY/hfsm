#ifndef HFSM_STATE_MACHINE_DEF_H_
#define HFSM_STATE_MACHINE_DEF_H_

#include <hfsm/state_machine_mpl.h>
#include <type_traits>

namespace hfsm {

struct normal_state_tag {};
struct pseudo_state_tag {};
struct state_machine_frontend_tag {};
struct state_machine_backend_tag {};

/**
 * @brief Base class for a regular state.
 *
 * Derive from this class to define a state that can remain active between
 * calls to state_machine::step().
 *
 * @tparam Derived The derived state type.
 */
template<typename Derived>
struct state {
  using state_flag = normal_state_tag;

  /** @brief Called when the state is entered. */
  void on_entry() {}

  /** @brief Called when the state remains active during an update. */
  void on_update() {}

  /** @brief Called when the state is exited. */
  void on_exit() {}
};

/**
 * @brief Base class for a transient pseudo state.
 *
 * A pseudo state is resolved immediately after it is entered. Outgoing
 * transitions are evaluated until a regular state is reached.
 *
 * A pseudo state does not remain active between calls to
 * state_machine::step(), therefore its on_update() callback is never invoked.
 *
 * @tparam Derived The derived pseudo-state type.
 */
template<typename Derived>
struct pseudo_state {
  using state_flag = pseudo_state_tag;

  /** @brief Called when the pseudo state is entered. */
  void on_entry() {}

  /**
   * @brief Update callback.
   *
   * This callback is never invoked for a pseudo state.
   */
  void on_update() {}

  /** @brief Called immediately before leaving the pseudo state. */
  void on_exit() {}
};

namespace detail {

template<typename State, typename Tag = void>
struct is_state : std::false_type {};

template<typename State>
struct is_state<State, hfsm::mpl::mp_void<typename State::state_flag>>
  : hfsm::mpl::mp_bool<
        std::is_same<typename State::state_flag, normal_state_tag>::value ||
        std::is_same<typename State::state_flag, pseudo_state_tag>::value ||
        std::is_same<typename State::state_flag, state_machine_frontend_tag>::value ||
        std::is_same<typename State::state_flag, state_machine_backend_tag>::value> {};

template<typename State, typename EnumClass, EnumClass EnumValue>
struct state_entry : public State {
  static_assert(detail::is_state<State>::value, "State must be a type representing a state or sub state machine");
  static_assert(std::is_enum<EnumClass>::value, "EnumClass must be an enum type");
  using state_type = State;
  using enum_type = EnumClass;
  static constexpr EnumClass enum_value = EnumValue;
};

template<typename T>
struct is_state_entry : std::false_type {};

template<typename State, typename EnumClass, EnumClass EnumValue>
struct is_state_entry<state_entry<State, EnumClass, EnumValue>> : std::true_type {};

} // namespace detail

/**
 * @brief Base class for defining a state machine.
 *
 * A state machine definition specifies the state enumeration, initial state,
 * transition table, guards, actions, and optional machine-level lifecycle
 * callbacks.
 *
 * Machine-level lifecycle callbacks apply to every state machine, including
 * the root state machine and nested state machines.
 *
 * @tparam Derived The derived state-machine definition type.
 * @tparam EnumClass Enumeration used to identify states in this machine.
 */
template<typename Derived, typename EnumClass>
struct state_machine_def {
  using state_flag = state_machine_frontend_tag;
  using enum_type = EnumClass;
  using guard_type = bool (Derived::*)();
  using action_type = void (Derived::*)();

  /**
   * @brief Associates a state type with an enumeration value.
   *
   * @tparam State State type or nested state-machine definition.
   * @tparam EnumValue Enumeration value identifying the state.
   */
  template<typename State, EnumClass EnumValue>
  using state_entry = detail::state_entry<State, EnumClass, EnumValue>;

  /**
   * @brief Called when this state machine is entered.
   *
   * For the root state machine, this callback is invoked by
   * state_machine::start(). For a nested state machine, it is invoked when
   * the parent machine enters that state.
   */
  void on_entry() {}

  /**
   * @brief Called whenever this state machine is updated.
   *
   * The callback is invoked before transitions at this machine level are
   * evaluated. If no transition is taken, the update is propagated to the
   * currently active substate.
   */
  void on_update() {}

  /**
   * @brief Called when this state machine is exited.
   *
   * This callback is invoked when a nested state machine is exited by its
   * parent. The root state machine currently has no stop operation, so its
   * on_exit() callback is not invoked during normal operation.
   */
  void on_exit() {}

  struct transition_entry_t {
    enum_type source;
    enum_type target;
    guard_type guard;
    action_type action;
  };

  /**
   * @brief Defines a transition between two states.
   *
   * When the source state is active and the guard evaluates to true, the
   * transition is taken in the following order:
   *
   * 1. Exit the source state.
   * 2. Execute the transition action.
   * 3. Enter the target state.
   *
   * @tparam Source Source state_entry type.
   * @tparam Target Target state_entry type.
   * @tparam Guard Guard member function of the state-machine definition.
   * @tparam Action Action member function of the state-machine definition.
   */
  template<typename Source, typename Target, guard_type Guard, action_type Action>
  struct transition {
    static_assert(
        detail::is_state_entry<Source>::value,
        "the transition's first template argument must be a state_entry type"
    );
    static_assert(
        detail::is_state_entry<Target>::value,
        "the transition's second template argument must be a state_entry type"
    );

    using source_state_entry_t = Source;
    using target_state_entry_t = Target;

    static constexpr transition_entry_t make_transition_entry() {
      constexpr auto source_state_enum_value = source_state_entry_t::enum_value;
      constexpr auto target_state_enum_value = target_state_entry_t::enum_value;
      return {source_state_enum_value, target_state_enum_value, Guard, Action};
    }
  };
};

namespace detail {

template<typename T>
class is_derived_from_state_machine_def {
 private:
  template<typename Derived, typename EnumClass>
  static std::true_type test(state_machine_def<Derived, EnumClass> const&);
  static std::false_type test(...);

 public:
  static constexpr bool value = decltype(test(std::declval<T>()))::value;
};

} // namespace detail

} // namespace hfsm

#endif /* HFSM_STATE_MACHINE_DEF_H_ */
