#include <paramify/paramify.hpp>
#include <iostream>

int main(int argc, char** argv) {
  paramify::Paramify params;

  if (!params.apply_cli_or_exit(argc, argv))
    return 0;

  std::cout << "audio_gain=" << (double)params["audio_gain"] << "\n";
  std::cout << "camera.enabled=" << (bool)params["camera.enabled"] << "\n";
  std::cout << "camera.device_id=" << (int64_t)params["camera.device_id"] << "\n";

  return 0;
}
