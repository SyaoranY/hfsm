#include <gtest/gtest.h>
#include <hfsm/state_machine_mpl.h>
#include <tuple>
#include <vector>

TEST(hfsm_mpl, mp_void) {
  using hfsm::mpl::mp_void;
  using L1 = mp_void<>;
  using L2 = mp_void<int>;
  using L3 = mp_void<int, int, double>;
}

TEST(hfsm_mpl, mp_list) {
  using hfsm::mpl::mp_list;
  using L1 = mp_list<>;
  using L2 = mp_list<int>;
  using L3 = mp_list<int, int, double>;
}

TEST(hfsm_mpl, mp_size) {
  using hfsm::mpl::mp_list;
  using hfsm::mpl::mp_size;
  using L1 = mp_list<>;
  using L2 = mp_list<int>;
  using L3 = mp_list<int, int, double>;
  using L4 = std::tuple<int, int, double>;
  static_assert(mp_size<L1>::value == 0);
  static_assert(mp_size<L2>::value == 1);
  static_assert(mp_size<L3>::value == 3);
  static_assert(mp_size<L4>::value == 3);
}

TEST(hfsm_mpl, mp_transform) {
  using hfsm::mpl::mp_list;
  using hfsm::mpl::mp_transform;

  using L1 = mp_list<>;
  using L2 = mp_list<int>;
  using L3 = mp_list<int, double, float>;
  using L4 = mp_transform<std::add_pointer_t, L1>;
  using L5 = mp_transform<std::add_pointer_t, L2>;
  using L6 = mp_transform<std::add_pointer_t, L3>;
  static_assert(std::is_same<L4, mp_list<>>::value);
  static_assert(std::is_same<L5, mp_list<int*>>::value);
  static_assert(std::is_same<L6, mp_list<int*, double*, float*>>::value);
}

TEST(hfsm_mpl, mp_apply) {
  using hfsm::mpl::mp_list;
  using hfsm::mpl::mp_apply;
  using L1 = mp_list<>;
  using L2 = mp_list<int>;
  using L3 = mp_list<int, double, float>;
  using L4 = mp_apply<std::tuple, L1>;
  using L5 = mp_apply<std::tuple, L2>;
  using L6 = mp_apply<std::tuple, L3>;
  static_assert(std::is_same<L4, std::tuple<>>::value);
  static_assert(std::is_same<L5, std::tuple<int>>::value);
  static_assert(std::is_same<L6, std::tuple<int, double, float>>::value);
}

TEST(hfsm_mpl, mp_append) {
  using hfsm::mpl::mp_list;
  using hfsm::mpl::mp_append;
  using L1 = mp_list<>;
  using L2 = mp_list<int>;
  using L3 = mp_list<int, double, float>;
  using L4 = mp_append<L1, L2>;
  static_assert(std::is_same<L4, mp_list<int>>::value);
  using L5 = mp_append<L2, L3>;
  static_assert(std::is_same<L5, mp_list<int, int, double, float>>::value);
  using L6 = mp_append<std::tuple<>, L1, L2, L3>;
  static_assert(std::is_same<L6, std::tuple<int, int, double, float>>::value);
}

TEST(hfsm_mpl, mp_set_contains) {
  using hfsm::mpl::mp_list;
  using hfsm::mpl::mp_set_contains;
  static_assert(!mp_set_contains<mp_list<>, int>::value);
  static_assert(mp_set_contains<mp_list<int>, int>::value);
  static_assert(mp_set_contains<mp_list<int, double>, int>::value);
  static_assert(mp_set_contains<mp_list<int, double, long>, double>::value);
  static_assert(mp_set_contains<mp_list<int, double, std::pair<int, double>>, std::pair<int, double>>::value);
  static_assert(mp_set_contains<std::tuple<int, double, long>, double>::value);
}

TEST(hfsm_mpl, mp_set_push_back) {
  using hfsm::mpl::mp_list;
  using hfsm::mpl::mp_set_push_back;
  using L1 = mp_list<>;
  using L2 = mp_set_push_back<L1, int>;
  static_assert(std::is_same<L2, mp_list<int>>::value);
  using L3 = mp_set_push_back<L2, double>;
  static_assert(std::is_same<L3, mp_list<int, double>>::value);
  using L4 = mp_set_push_back<L3, long, long long>;
  static_assert(std::is_same<L4, mp_list<int, double, long, long long>>::value);
  using L5 = mp_set_push_back<L4, int>;
  static_assert(std::is_same<L5, L4>::value);
  using L6 = mp_set_push_back<L5, int, long long>;
  static_assert(std::is_same<L6, L4>::value);
}

TEST(hfsm_mpl, mp_set_union) {
  using hfsm::mpl::mp_list;
  using hfsm::mpl::mp_set_union;
  using L1 = mp_list<>;
  using L2 = std::pair<int, double>;
  using L3 = std::tuple<double, float>;
  using L4 = mp_list<std::vector<int>, std::vector<double>, std::vector<float>>;
  static_assert(std::is_same<mp_set_union<L1, L2>, mp_list<int, double>>::value);
  static_assert(std::is_same<mp_set_union<L3, L2>, std::tuple<double, float, int>>::value);
  using L5 = mp_list<int, double, float, std::vector<int>, std::vector<double>, std::vector<float>>;
  static_assert(std::is_same<mp_set_union<L1, L2, L3, L4>, L5>::value);
}

TEST(hfsm_mpl, mp_unique) {
  using hfsm::mpl::mp_list;
  using hfsm::mpl::mp_unique;
  using L1 = mp_unique<mp_list<>>;
  static_assert(std::is_same<L1, mp_list<>>::value);
  using L2 = mp_unique<mp_list<int>>;
  static_assert(std::is_same<L2, mp_list<int>>::value);
  using L3 = mp_unique<mp_list<int, double, long>>;
  static_assert(std::is_same<L3, mp_list<int, double, long>>::value);
  using L4 = mp_unique<mp_list<int, double, long, int, float>>;
  static_assert(std::is_same<L4, mp_list<int, double, long, float>>::value);
  using L5 = mp_unique<mp_list<int, int, int, int>>;
  static_assert(std::is_same<L5, mp_list<int>>::value);
}

TEST(hfsm_mpl, tuple_visit_if) {
  using hfsm::mpl::tuple_visit_if;
  { // visits first matched element
    std::tuple<int, double, char> tuple{1, 2.5, 'a'};

    bool visited = false;

    const bool found = tuple_visit_if(
        tuple,
        [](auto const& value) {
            return std::is_same<
                typename std::decay<decltype(value)>::type,
                double
            >::value;
        },
        [&](auto& value) {
            visited = true;
            EXPECT_DOUBLE_EQ(value, 2.5);
        });

    EXPECT_TRUE(found);
    EXPECT_TRUE(visited);
  }
  { // returns false when no element matches
    std::tuple<int, double, char> tuple{1, 2.5, 'a'};

    bool visited = false;

    const bool found = tuple_visit_if(
        tuple,
        [](auto const&) {
            return false;
        },
        [&](auto&) {
            visited = true;
        });

    EXPECT_FALSE(found);
    EXPECT_FALSE(visited);
  }
  { // stop after first match
    std::tuple<int, int, int> tuple{1, 2, 3};

    int predicate_count = 0;
    int visit_count = 0;

    const bool found = tuple_visit_if(
        tuple,
        [&](int value) {
            ++predicate_count;
            return value == 2;
        },
        [&](int& value) {
            ++visit_count;
            EXPECT_EQ(value, 2);
        });

    EXPECT_TRUE(found);
    EXPECT_EQ(predicate_count, 2);
    EXPECT_EQ(visit_count, 1);
  }
  { // can modify matched element
    std::tuple<int, int, int> tuple{1, 2, 3};

    const bool found = tuple_visit_if(
        tuple,
        [](int value) {
            return value == 2;
        },
        [](int& value) {
            value = 20;
        });

    EXPECT_TRUE(found);
    EXPECT_EQ(std::get<0>(tuple), 1);
    EXPECT_EQ(std::get<1>(tuple), 20);
    EXPECT_EQ(std::get<2>(tuple), 3);
  }
}