#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <paramify/paramify.hpp>
#include <fstream>
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

TEST_CASE("YAML load: primitives + defaults", "[core]") {
  const auto path = write_temp_yaml(R"(
name: test
description: test

parameters:
  - name: gain
    type: double
    default: 1.0
    scope: all

  - name: use_camera
    type: bool
    default: true
    scope: cli

  - name: robot_name
    type: string
    default: qtrobot
    scope: all

  - name: device_id
    type: int
    default: 2
)");

  paramify::Paramify p(path);

  REQUIRE((double)p["gain"] == Approx(1.0));
  REQUIRE((bool)p["use_camera"] == true);
  REQUIRE((std::string)p["robot_name"] == "qtrobot");
  REQUIRE((int64_t)p["device_id"] == 2);

  std::remove(path.c_str());
}

TEST_CASE("YAML load: list defaults", "[core]") {
  const auto path = write_temp_yaml(R"(
parameters:
  - name: languages
    type: list[string]
    default: ["en", "fr"]
    scope: all

  - name: nums
    type: list[int]
    default: [1, 2, 3]
    scope: all
)");

  paramify::Paramify p(path);

  auto langs = (std::vector<std::string>)p["languages"];
  REQUIRE(langs.size() == 2);
  REQUIRE(langs[0] == "en");
  REQUIRE(langs[1] == "fr");

  auto nums = (std::vector<int64_t>)p["nums"];
  REQUIRE(nums == std::vector<int64_t>{1,2,3});

  std::remove(path.c_str());
}

TEST_CASE("Map-style write then read back", "[core]") {
  paramify::Paramify p;

  p.add_param("gain", paramify::ValueType::Double, 1.0);
  p.add_param("enabled", paramify::ValueType::Bool, true);
  p.add_param("name", paramify::ValueType::String, std::string("qtrobot"));

  p["gain"] = 2.5;
  p["enabled"] = false;
  p["name"] = "bob";

  REQUIRE((double)p["gain"] == Approx(2.5));
  REQUIRE((bool)p["enabled"] == false);
  REQUIRE((std::string)p["name"] == "bob");
}

TEST_CASE("Unset param (no default) throws on read", "[core]") {
  const auto path = write_temp_yaml(R"(
parameters:
  - name: x
    type: int
    scope: all
)");

  paramify::Paramify p(path);
  REQUIRE_THROWS_AS((int64_t)p["x"], paramify::ParamNotFound);

  std::remove(path.c_str());
}

TEST_CASE("Type mismatch throws", "[core]") {
  paramify::Paramify p;
  p.add_param("gain", paramify::ValueType::Double, 1.0);

  REQUIRE_THROWS_AS((std::string)p["gain"], paramify::ParamTypeError);
  REQUIRE_THROWS_AS((bool)p["gain"], paramify::ParamTypeError);

  // assignment mismatch should throw
  REQUIRE_THROWS_AS((p["gain"] = std::string("nope")), paramify::ParamTypeError);
}

TEST_CASE("Assignment widening int->double allowed (schema double)", "[core]") {
  paramify::Paramify p;
  p.add_param("gain", paramify::ValueType::Double, 1.0);

  // int literal routes to int64_t assignment then allowed to widen to double internally
  p["gain"] = 3;
  REQUIRE((double)p["gain"] == Approx(3.0));
}
