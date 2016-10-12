cc_library(
   name = 'colib',
   target_bases = [ 'comm',],
   srcs = [
        'co_hook_sys_call.cpp',
        'co_routine.cpp',
		'coctx.cpp',
		'coctx_swap.S',
		'co_epoll.cpp',

   ],
   deps = [
   ],
   defs = [
        'LINUX',
        '_PTHREADS',
         'TIXML_USE_STL',
         ],
   optimize = [
#'O2',
   ],
   extra_cppflags = [ 
#'-O2' , 
        #'-Werror',
    ],
)
cc_binary(
    name = 'global_sym',
    srcs = [ 
        'demangle.cpp',
		'global_sym.cpp'

    ],
    deps = [ 


#GEN_SOURCE_POS__
    ],
    defs = [
		'LINUX',
        '_PTHREADS',
        'TIXML_USE_STL',
        '_NEW_LIC',
        '_GNU_SOURCE',
        '_REENTRANT',

           ],
	optimize = [
'O2',
		],
    extra_cppflags = [ 
        #'-Werror',
        #'-Werror',
        '-pipe',
        '-fPIC',
        '-Wno-deprecated',

    ],
	incs = [ 
#INCS_POS__
	],
)
cc_binary(
    name = 'test_specific',
    srcs = [ 
        'test_specific.cpp',

    ],
    deps = [ 
		':colib',
		'//comm2/core:core',

#GEN_SOURCE_POS__
    ],
    defs = [
		'LINUX',
        '_PTHREADS',
        'TIXML_USE_STL',
        '_NEW_LIC',
        '_GNU_SOURCE',
        '_REENTRANT',

           ],
	optimize = [
'O2',
		],
    extra_cppflags = [ 
        #'-Werror',
        #'-Werror',
        '-pipe',
        '-fPIC',
        '-Wno-deprecated',

    ],
	incs = [ 
#INCS_POS__
	],
)
cc_binary(
    name = 'echosvr',
    srcs = [ 
        'example_echosvr_copystack.cpp',

    ],
    deps = [ 
		':colib',
		'//comm2/core:core',

#GEN_SOURCE_POS__
    ],
    defs = [
		'LINUX',
        '_PTHREADS',
        'TIXML_USE_STL',
        '_NEW_LIC',
        '_GNU_SOURCE',
        '_REENTRANT',

           ],
	optimize = [
'O2',
		],
    extra_cppflags = [ 
        #'-Werror',
        #'-Werror',
        '-pipe',
        '-fPIC',
        '-Wno-deprecated',

    ],
	incs = [ 
#INCS_POS__
	],
)
cc_binary(
   name = 'test_colib',
    srcs = [ 
'test_colib.cpp',
 'test_xmm.S',
    ],
    deps = [ 
':colib',
'//mm3rd/gtest:gtest',
'#dl',
#GEN_SOURCE_POS__
    ],
    defs = [
           ],
	optimize = [
'O2',
		],
    extra_cppflags = [ 
        #'-Werror',
        #'-Werror',
'-pipe',
        '-fPIC',
        '-Wno-deprecated',
		'-DLIBCO-USE-GETTICK',

    ],
	incs = [ 
#INCS_POS__
	],
)
