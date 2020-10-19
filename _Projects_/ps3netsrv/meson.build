project('ps3netsrv', ['c', 'cpp'],
  license : 'GPL3',
  version : '0.1',
  default_options : [
    'warning_level=3',
    'c_std=c99',
    'cpp_std=c++11'
    ]
)

ps3netsrv_inc = include_directories('include')
ps3netsrv_src = files(
  'src/compat.c',
  'src/File.cpp',
  'src/main.cpp',
  'src/VIsoFile.cpp',
  'src/Console.cpp'
)
if host_machine.system() == 'windows'
  ps3netsrv_src += files(
    'src/scandir.c',
    'src/dirent.c'
  )
endif

thread_dep = dependency('threads')
cc = meson.get_compiler('c')
mbedtls_dep = cc.find_library('mbedtls')
mbedcrypto_dep = cc.find_library('mbedcrypto')

ps3netsrv_exe = executable('ps3netsrv', ps3netsrv_src,
  dependencies : [
    thread_dep,
    mbedtls_dep,
    mbedcrypto_dep
  ],
  include_directories : ps3netsrv_inc,
  install : true
)
