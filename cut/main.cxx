#include <args/args.hh>
#include <cloud/cloud.hh>
#include <input/input.hh>
#include <generator/generator.hh>
#include <parser/parser.hh>
#include <processing/processing.hh>

#include <map>
#include <chrono>
#include <cmath>

int main(int argc, char** argv) {
  const std::string table = "main";
  std::cout << "objectcut" << std::endl;
  cloud::Cloud cl;
  if (!cl.init()) {
    std::cerr << "Bad initialization" << std::endl;
    return 1;
  }
  gen::Generator g;
  pro::Proc p;
  // while (true) {
  {
    const auto [source, vertices] = cl.parse(cl.receive());
    const auto obj = p(cl.image(source), vertices);
    const std::string ext = ".jpg";
    std::vector<unsigned char> bytes;
    const auto encoded = cv::imencode(ext, obj, bytes);
    if (!encoded) {
      return 3;
    }
    std::stringstream ss;
    for (auto c : bytes) {
      ss << c;
    }
    std::string data = ss.str();
    const auto sto = cl.put(data);
    if (!sto.has_value()) {
      return 4;
    }
    auto dbs = cl.tableExists(table);
    if (!dbs) {
      dbs = cl.tableCreate(table);
      if (!dbs) {
        return 5;
      }
    }
    dbs = cl.tableSave(table, sto.value(), source);
  }
  if (!cl.deinit()) {
    std::cerr << "Bad deinitialization" << std::endl;
    return 2;
  }

  return 0;
}
