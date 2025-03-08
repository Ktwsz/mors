project('mors-cli', 'cpp')

front_proj = subproject('mors-frontend')
front_dep = front_proj.get_variable('front_dep')

executable('mors-cli', 'main/main.cpp', dependencies: front_dep)

