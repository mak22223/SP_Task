add_executable(master
  src/main.cpp
  src/Master.cpp
)

target_include_directories(master PUBLIC
  include
)

# TODO: install executables to install directory
