#include <paramify/paramify.hpp>
#include <iostream>

int main(int argc, char** argv) {
  paramify::Paramify params;

  params.add_param("audio_gain",
                   paramify::ValueType::Double,
                   1.0,
                   paramify::Scope::Cli,
                   "Audio gain multiplier");

  params.add_param("camera.enabled",
                   paramify::ValueType::Bool,
                   true,
                   paramify::Scope::Cli,
                   "Enable camera");

  params.add_param("camera.device_id",
                   paramify::ValueType::Int,
                   (int64_t)0,
                   paramify::Scope::Runtime,
                   "Camera device id");

  if (!params.apply_cli_or_exit(argc, argv))
    return 0;

  std::cout << "audio_gain=" << (double)params["audio_gain"] << "\n";
  std::cout << "camera.enabled=" << (bool)params["camera.enabled"] << "\n";
  std::cout << "camera.device_id=" << (int64_t)params["camera.device_id"] << "\n";

  return 0;
}
