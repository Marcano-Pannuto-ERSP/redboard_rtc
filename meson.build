project('redboard_rtc', 'c',
  default_options: [
    'b_lto=true',
    'b_staticpic=false',
    'c_std=c2x',
    'warning_level=3',
  ],
  license: ['Apache-2.0'],
  version: '0.1.0')

c_args = [
  '-ffunction-sections',
]

link_args = [
  '-Wl,--gc-sections', '-fno-exceptions',
]

# This build script is configured to build all of the non-main code as a
# library, and main.c as an executable that links in the aforementioned library

# This following section on finding libm is only needed if you need to use
# math.h functions
cc = meson.get_compiler('c', native: false)
m_dep = cc.find_library('m', required : false)

# Adjust these libraries to use the right version for the board in use. The
# defaults here are for the Redboard ATP.
ambiq_lib = dependency('ambiq_rba_atp')
asimple_lib = dependency('asimple_rba_atp')

# This section is for building most of the program as a library
lib_sources = files([
  'src/rtc.c',
])

includes = include_directories([
  'include/rtc',
])

install_library = false

lib = library(meson.project_name(),
  lib_sources,
  dependencies: [ambiq_lib, asimple_lib],
  include_directories: includes,
  c_args: c_args,
  link_args: link_args,
  install: install_library, # Change this to false if you do not want to install library
)

install_subdir('include', install_dir: '')

# Create a pkgconfig file

if (install_library == true)
  pkg = import('pkgconfig')
  pkg.generate(lib, subdirs: ['', 'rtc'])
endif

# Section defining the executable
sources = files([
  'src/main.c',
])

exe = executable(meson.project_name(),
  sources,
  link_with: lib,
  dependencies: [ambiq_lib, m_dep, asimple_lib],
  include_directories: includes,
  c_args: c_args,
  link_args: link_args + ['-T' + meson.source_root() / 'linker.ld']
)

# Custom flash target, to make it easier to program the RedBoard, requires
# objcopy
objcopy = find_program('objcopy')

bin = custom_target(
  input : exe,
  output : exe.name().split('.')[0] + '.bin',
  command : [objcopy, '-O', 'binary', '@INPUT@', '@OUTPUT@', ],
  build_by_default: true
)

run_target('flash',
  command : ['python3', meson.source_root() / 'svl.py',
    get_option('tty'), '-f',  bin, '-b', '921600', '-v'],
  depends : bin,
)
