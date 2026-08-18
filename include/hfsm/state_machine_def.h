#ifndef HFSM_STATE_MACHINE_DEF_H_
#define HFSM_STATE_MACHINE_DEF_H_

#include <type_traits>
#include <hfsm/state_machine_mpl.h>

namespace hfsm {

struct normal_state_tag { };
struct state_machine_frontend_tag { };
struct state_machine_backend_tag { };

template<typename Derived>
struct state {
  using state_flag = normal_state_tag;
  void on_entry() { }
  void on_update() { }
  void on_exit() { }
};

namespace detail {

template<typename State, typename Tag = void>
struct is_state : std::false_type { };

template<typename State>
struct is_state<State, hfsm::mpl::mp_void<typename State::state_flag>> 
: hfsm::mpl::mp_bool<std::is_same<typename State::state_flag, normal_state_tag>::value
                  || std::is_same<typename State::state_flag, state_machine_frontend_tag>::value
                  || std::is_same<typename State::state_flag, state_machine_backend_tag>::value> {
};

template<typename State, typename EnumClass, EnumClass EnumValue>
struct state_entry : public State {
  static_assert(detail::is_state<State>::value, "State must be a type representing a state or sub state machine");
  static_assert(std::is_enum<EnumClass>::value, "EnumClass must be an enum type");
  using value_type = State;
  using enum_type = EnumClass;
  static constexpr EnumClass enum_value = EnumValue;
};

template<typename T>
struct is_state_entry : std::false_type { };

template<typename State, typename EnumClass, EnumClass EnumValue>
struct is_state_entry<state_entry<State, EnumClass, EnumValue>> : std::true_type { };


} // namespace detail

template<typename Derived, typename EnumClass>
struct state_machine_def {
  using state_flag  = state_machine_frontend_tag;
  using enum_type   = EnumClass;
  using guard_type  = bool(Derived::*)();
  using action_type = void(Derived::*)();

  template<typename State, EnumClass EnumValue>
  using state_entry = detail::state_entry<State, EnumClass, EnumValue>;

  void on_entry() { }
  void on_update() { }
  void on_exit() { }

  // transition runtime entry type
  struct transition_entry_t {
    enum_type source;
    enum_type target;
    guard_type guard;
    action_type action;
  };
  // using transition_entry_t = std::tuple<enum_type, enum_type, guard_type, action_type>;

  template<typename source, typename target, guard_type guard_value, action_type action_value>
  struct transition {
    static_assert(detail::is_state_entry<source>::value, "the transition's first template argument must be a state_entry type");
    static_assert(detail::is_state_entry<target>::value, "the transition's second template argument must be a state_entry type");

    using source_state_entry_t = source;
    using target_state_entry_t = target;
    // using source_state = typename source_state_entry_t::value_type;
    // using target_state = typename target_state_entry_t::value_type;
    
    static constexpr transition_entry_t make_transition_entry() {
      constexpr auto source_state_enum_value = source_state_entry_t::enum_value;
      constexpr auto target_state_enum_value = target_state_entry_t::enum_value;
      return {source_state_enum_value, target_state_enum_value, guard_value, action_value};
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

} // namespace

} // namespace hfsm

#endif /* HFSM_STATE_MACHINE_DEF_H_ */
