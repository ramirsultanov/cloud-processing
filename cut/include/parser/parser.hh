#ifndef PARSER_HH_
#define PARSER_HH_

#include <tuple>
#include <string>
#include <stdexcept>
#include <array>
#include <sstream>

namespace par {

class Parser {
public:
  std::tuple<long long, long long, unsigned>
  operator()(const std::string&) const;
  constexpr static auto map = std::array{"+", "-", "*", "/", "^"};
protected:
private:
};

} /// namespace par

namespace par {

std::tuple<long long, long long, unsigned>
Parser::operator()(const std::string& exp) const {
  std::stringstream ss(exp);
  std::string tmp;
  ss >> tmp;
  const auto a = std::stoll(tmp);
  ss >> tmp;
  const auto sign = tmp;
  ss >> tmp;
  const auto b = std::stoll(tmp);
  const auto o = std::find(map.begin(), map.end(), sign);
  if (o == map.end()) {
    throw std::runtime_error("operator is not supported");
  }
  return {a, b, std::distance(map.begin(), o)};
}

} /// namespace par

#endif /// PARSER_HH_
