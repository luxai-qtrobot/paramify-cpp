#include <paramify/paramify.hpp>
#include <iostream>

int main(int argc, char** argv) {
  paramify::Paramify params;

  if (!params.apply_cli_or_exit(argc, argv))
    return 0;

  std::cout << "gain=" << (double)params["gain"] << "\n";
  std::cout << "use_camera=" << (bool)params["use_camera"] << "\n";

  return 0;
}
