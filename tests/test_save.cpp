#include <catch2/catch.hpp>

#include <paramify/paramify.hpp>
#include <fstream>
#include <sstream>
#include <cstdio>

static std::string write_temp_yaml(const std::string& content) {
  char buf[L_tmpnam];
  std::tmpnam(buf);
  std::string path = std::string(buf) + ".yaml";
  std::ofstream f(path);
  REQUIRE(f.good());
  f << content;
  return path;
}

static std::string read_file(const std::string& path) {
  std::ifstream f(path);
  REQUIRE(f.good());
  std::ostringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

TEST_CASE("save_to_file writes updated defaults", "[save]") {
  const auto path = write_temp_yaml(R"(
parameters:
  - name: gain
    type: double
    default: 1.0
    scope: all

  - name: use_camera
    type: bool
    default: true
    scope: cli
)");

  paramify::Paramify p(path);

  p["gain"] = 2.5;
  p["use_camera"] = false;

  // Persist
  p.save_to_file();

  // Reload & verify defaults updated
  paramify::Paramify p2(path);
  REQUIRE((double)p2["gain"] == Approx(2.5));
  REQUIRE((bool)p2["use_camera"] == false);

  std::remove(path.c_str());
}
