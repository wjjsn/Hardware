hardware_src = files('OLED/font.cpp')

hardware_dep = declare_dependency(
  sources: hardware_src,
  include_directories: include_directories('.'),
)

meson.override_dependency('Hardware', hardware_dep)
