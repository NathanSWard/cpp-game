#include "nova/util/type_map.hpp"

using namespace nova;

int main() {}

/*
TEST(type_map, basic_functionality) {
  feature("basic functionality") = [] {
    given("I have an empty map") = [] {
      TypeMap map{};

      then("the map should report as empty") = [&] {
        expect(0u == map.size());
        expect(map.empty());
      };

      then("get<T>() should return a nullopt") = [&] {
        expect(not map.get<int>().has_value());
        expect(not map.get<my_type>().has_value());
      };

      then("remove<T>() should return a nullopt") = [&] {
        expect(not map.remove<int>().has_value());
        expect(not map.remove<my_type>().has_value());
      };

      then("contains<T>() should return false") = [&] {
        expect(not map.contains<int>());
        expect(not map.contains<my_type>());
      };

      when("I try to add a type `int` into the map") = [&] {
        const auto [value, inserted] = map.try_add<int>(42);

        then("the value is inserted") = [&] {
          expect(inserted);
          expect(value == 42);
        };

        then("the map contains the value") = [&] {
          expect(map.contains<int>());
          expect(1u == map.size());
          expect(not map.empty());
        };
      };

      when("I try to get the value") = [&] {
        const auto value = map.get<int>();

        then("the value is returned") = [&] {
          expect(value.has_value() >> fatal);
          expect(*value == 42);
        };
      };

      when("I try to overwrite the value via `try_add`") = [&] {
        const auto [value, inserted] = map.try_add<int>(100);

        then("the new value is NOT inserted") = [&] {
          expect(not inserted);
          expect(value == 42);
        };
      };

      when("I try to overwrite the value via `set`") = [&] {
        const auto value = map.set<int>(100);

        then("the new value IS inserted") = [&] { expect(value == 100); };
      };

      when("I remove the value") = [&] {
        const auto value = map.remove<int>();

        then("the value is returned") = [&] {
          expect(value.has_value() >> fatal);
          expect(*value == 100);
        };

        then("the map no longer contains the value") = [&] {
          expect(not map.contains<int>());
          expect(0u == map.size());
          expect(map.empty());
        };
      };
    };
  };
}

struct my_type {};
struct destructor_check {
  inline static std::size_t destroyed{};
  inline static std::size_t moved{};
  destructor_check() = default;
  destructor_check(destructor_check&&) noexcept { ++moved; }
  ~destructor_check() { ++destroyed; }
};

int main() {
  "[type_map]"_test = [] {


    feature("destructor check") = [] {
      given("I have a pre-populated map") = [] {
        TypeMap map{};
        map.set<destructor_check>();
        expect(0u == destructor_check::destroyed);
        expect(0u == destructor_check::moved);

        when("i remove the value") = [&] {
          auto value = map.remove<destructor_check>();

          then("i expect a move ctor and destructor to have been run") = [&] {
            expect(1u == destructor_check::moved);
            expect(1u == destructor_check::destroyed);
          };
        };
      };
    };
  };

 */
