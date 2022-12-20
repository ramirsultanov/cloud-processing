#ifndef GENERATOR_HH_
#define GENERATOR_HH_

#include <string>
#include <random>
#include <array>

namespace gen {

class Generator {
public:
  std::string operator()();
protected:
  std::random_device rd_;
  std::mt19937 gen_{rd_()};
  std::uniform_int_distribution<short> d_{0, 10};
  constexpr static auto map_ = std::array{"+", "-", "*", "/", "^"};
private:
};

} /// namespace gen

namespace gen {

std::string Generator::operator()() {
  const auto a = d_(gen_);
  const auto b = d_(gen_);
  const auto o = map_[d_(gen_) % map_.size()];
  return std::to_string(a) + " " + o + " " + std::to_string(b);
}

} /// namespace gen

#endif /// GENERATOR_HH_
