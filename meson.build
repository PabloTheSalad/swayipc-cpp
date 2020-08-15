project('swayipc-cpp', 'cpp',
        default_options : ['cpp_std=c++17'])

incdir = include_directories('include')

src = ['./src/connection.cpp', './src/socket_wrapper.cpp', './src/tree.cpp']

json_dep = dependency('nlohmann_json')

swayipc_lib = static_library('swayipc-cpp',
                             src,
                             include_directories : incdir,
                             dependencies : json_dep,
                             override_options : ['cpp_std=c++17'])

swayipc_dep = declare_dependency(link_with : swayipc_lib, include_directories : incdir)