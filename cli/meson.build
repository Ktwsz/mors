project('mors-cli', 'cpp', default_options: [
  'cpp_std=c++23',
  'buildtype=release',
  'warning_level=3'
])


front_proj = subproject('mors-frontend')
front_dep = front_proj.get_variable('front_dep')

executable('mors-cli', 'main/main.cpp', dependencies: front_dep)

