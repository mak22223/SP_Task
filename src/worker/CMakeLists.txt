add_executable(worker
  src/main.cpp
  src/Worker.cpp
)

target_include_directories(worker PUBLIC
  include
)

# TODO: install executables to install directory
