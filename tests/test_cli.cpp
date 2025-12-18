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

static std::vector<char*> make_argv(std::vector<std::string>& args) {
  std::vector<char*> argv;
  argv.reserve(args.size());
  for (auto& s : args) argv.push_back(&s[0]);
  return argv;
}

TEST_CASE("CLI: --config loads yaml and overrides values", "[cli]") {
  const auto path = write_temp_yaml(R"(
parameters:
  - name: gain
    type: double
    default: 1.0
    scope: all
    description: Gain

  - name: use_camera
    type: bool
    default: true
    scope: cli
    description: Camera

  - name: robot_name
    type: string
    default: qtrobot
    scope: all
)");

  paramify::Paramify p;

  std::vector<std::string> args = {
    "app",
    "--config", path,
    "--gain", "2.5",
    "--no-use-camera",
    "--robot-name", "bob"
  };
  auto argv = make_argv(args);
  int argc = (int)argv.size();

  REQUIRE(p.apply_cli_or_exit(argc, argv.data()) == true);

  REQUIRE((double)p["gain"] == Approx(2.5));
  REQUIRE((bool)p["use_camera"] == false);
  REQUIRE((std::string)p["robot_name"] == "bob");

  std::remove(path.c_str());
}

TEST_CASE("CLI: list override", "[cli]") {
  const auto path = write_temp_yaml(R"(
parameters:
  - name: languages
    type: list[string]
    default: ["en"]
    scope: all
    description: Languages
)");

  paramify::Paramify p;

  std::vector<std::string> args = {
    "app",
    "--config", path,
    "--languages", "en", "de", "fr"
  };
  auto argv = make_argv(args);
  int argc = (int)argv.size();

  REQUIRE(p.apply_cli_or_exit(argc, argv.data()) == true);

  auto langs = (std::vector<std::string>)p["languages"];
  REQUIRE(langs == std::vector<std::string>{"en","de","fr"});

  std::remove(path.c_str());
}

TEST_CASE("CLI: programmatic schema + overrides", "[cli]") {
  paramify::Paramify p;
  p.add_param("gain", paramify::ValueType::Double, 1.0, paramify::Scope::Cli, "Gain");
  p.add_param("enabled", paramify::ValueType::Bool, true, paramify::Scope::All, "Enabled");

  std::vector<std::string> args = {"app", "--gain", "4.0", "--no-enabled"};
  auto argv = make_argv(args);
  int argc = (int)argv.size();

  REQUIRE(p.apply_cli_or_exit(argc, argv.data()) == true);
  REQUIRE((double)p["gain"] == Approx(4.0));
  REQUIRE((bool)p["enabled"] == false);
}

TEST_CASE("CLI: apply_cli_or_exit returns false on --help", "[cli]") {
  // With programmatic schema, help should still work and return false
  paramify::Paramify p;
  p.add_param("gain", paramify::ValueType::Double, 1.0, paramify::Scope::Cli, "Gain");

  std::vector<std::string> args = {"app", "--help"};
  auto argv = make_argv(args);
  int argc = (int)argv.size();

  REQUIRE(p.apply_cli_or_exit(argc, argv.data()) == false);
}
