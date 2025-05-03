project('mors-frontend', 'cpp')

includes = include_directories('main')
includes_mzn = include_directories('libminizinc/include', 'libminizinc/build/include')

object_grabber_cmd = run_command('grabber.sh', check: true)
mzn_objects = object_grabber_cmd.stdout().strip().split('\n')

front_lib = static_library('mors-frontend',
  'main/lib.cpp',
  include_directories: includes_mzn,
  objects: mzn_objects)

front_dep = declare_dependency(link_with: front_lib, include_directories: includes)
