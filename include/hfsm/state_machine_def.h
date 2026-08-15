#ifndef HFSM_STATE_MACHINE_DEF_H_
#define HFSM_STATE_MACHINE_DEF_H_

#include <hfsm/state_machine_mpl.h>

namespace hfsm {

struct normal_state_tag { };
struct state_machine_tag { };

template<typename Derived>
struct state {
  using state_flag = normal_state_tag;
  void on_entry() {
    // TBD: cheeck if Derived has on_entry
    static_cast<Derived*>(this)->on_entry();
  }

  void on_update() {
    // TBD: cheeck if Derived has on_update
    static_cast<Derived*>(this)->on_update();
  }

  void on_exit() {
    // TBD: cheeck if Derived has on_exit
    static_cast<Derived*>(this)->on_exit();
  }
};

namespace detail {

template<typename State, typename Tag = void>
struct is_state : std::false_type { };

template<typename State>
struct is_state<State, hfsm::mpl::mp_void<typename State::state_flag>> 
: std::bool_constant<std::is_same<typename State::state_flag, normal_state_tag>::value
                  || std::is_same<typename State::state_flag, state_machine_tag>::value> {
};

template<typename State, typename EnumClass, EnumClass EnumValue>
struct state_ref {
  static_assert(detail::is_state<State>::value, "State must be a type representing a state or sub state machine");
  static_assert(std::is_enum<EnumClass>::value, "EnumClass must be an enum type");
  using value_type = State;
  static constexpr EnumClass enum_value = EnumValue;
};

template<typename T>
struct is_state_ref : std::false_type { };

template<typename State, typename EnumClass, EnumClass EnumValue>
struct is_state_ref<state_ref<State, EnumClass, EnumValue>> : std::true_type { };


} // namespace detail

template<typename Derived, typename EnumClass>
struct state_machine_def {
  using state_flag  = state_machine_tag;
  using enum_type   = EnumClass;
  using guard_type  = bool(Derived::*)();
  using action_type = void(Derived::*)();

  template<typename State, EnumClass EnumValue>
  using state_ref = detail::state_ref<State, EnumClass, EnumValue>;

  template<typename source, typename target, guard_type guard_value, action_type action_value>
  struct transition {
    static_assert(detail::is_state_ref<source>::value, "the transition's first template argument must be a state_ref type");
    static_assert(detail::is_state_ref<target>::value, "the transition's second template argument must be a state_ref type");

    using source_state_ref_t = source;
    using target_state_ref_t = target;
    using source_state = typename source_state_ref_t::value_type;
    using target_state = typename target_state_ref_t::value_type;

    using entry_type = std::tuple<enum_type, enum_type, guard_type, action_type>;
    
    static constexpr auto make_transition_entry() -> entry_type {
      constexpr auto source_state_enum_value = source_state_ref_t::enum_value;
      constexpr auto target_state_enum_value = target_state_ref_t::enum_value;
      return {source_state_enum_value, target_state_enum_value, guard_value, action_value};
    }
  };
};


} // namespace hfsm

#endif /* HFSM_STATE_MACHINE_DEF_H_ */
