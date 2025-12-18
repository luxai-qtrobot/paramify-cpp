#include <paramify/paramify.hpp>
#include <iostream>

int main() {
  paramify::Paramify params("params.yaml");

  double gain = params["gain"];
  bool cam = params["use_camera"];

  std::cout << "gain=" << gain
            << " use_camera=" << cam << "\n";

  params["gain"] = 2.0;
  std::cout << "new gain=" << (double)params["gain"] << "\n";

  return 0;
}
