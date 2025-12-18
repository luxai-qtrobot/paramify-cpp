#include <paramify/paramify.hpp>
#include <iostream>

int main(int argc, char** argv) {
  paramify::Paramify params;

  params.add_param("gain",
                   paramify::ValueType::Double,
                   1.0,
                   paramify::Scope::Cli,
                   "Gain multiplier");

  params.add_param("enabled",
                   paramify::ValueType::Bool,
                   true,
                   paramify::Scope::All,
                   "Enable feature");

  if (!params.apply_cli_or_exit(argc, argv))
    return 0;


  std::cout << "gain=" << (double)params["gain"] << "\n";
  std::cout << "enabled=" << (bool)params["enabled"] << "\n";

  return 0;
}
