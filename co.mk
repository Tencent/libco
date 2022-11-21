#
# Tencent is pleased to support the open source community by making Libco available.
#
# Copyright (C) 2014 THL A29 Limited, a Tencent company. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License"); 
# you may not use this file except in compliance with the License. 
# You may obtain a copy of the License at
#
#	http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, 
# software distributed under the License is distributed on an "AS IS" BASIS, 
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
# See the License for the specific language governing permissions and 
# limitations under the License.
#



##### Makefile Rules ##########
MAIL_ROOT=.
SRCROOT=.

##define the compliers
CPP = $(CXX)
AR = ar -rc
RANLIB = ranlib

CPPSHARE = $(CPP) -fPIC -shared -O2 -pipe -L$(SRCROOT)/solib/ -o 
CSHARE = $(CC) -fPIC -shared -O2 -pipe -L$(SRCROOT)/solib/ -o 

ifeq ($v,release)
CFLAGS= -O2 $(INCLS) -fPIC  -DLINUX -pipe -Wno-deprecated -c
else
CFLAGS= -g $(INCLS) -fPIC -DLINUX -pipe -c -fno-inline
endif

ifneq ($v,release)
BFLAGS= -g
endif

BUILDDIR = build
STATICLIBPATH=$(SRCROOT)/$(BUILDDIR)/lib
DYNAMICLIBPATH=$(SRCROOT)/$(BUILDDIR)/solib

INCLS += -I$(SRCROOT)

## default links
ifeq ($(LINKS_DYNAMIC), 1)
LINKS += -L$(DYNAMICLIBPATH) -L$(STATICLIBPATH)
else
LINKS += -L$(STATICLIBPATH)
endif

CPPSRCS  = $(wildcard *.cpp) 
CSRCS  = $(wildcard *.c)
SSRCS  = $(wildcard *.S)
CPPOBJS  = $(patsubst %.cpp,$(BUILDDIR)/%.o,$(CPPSRCS))
COBJS  = $(patsubst %.c,$(BUILDDIR)/%.o,$(CSRCS))
SOBJS  = $(patsubst %.S,$(BUILDDIR)/%.o,$(SSRCS))

SRCS = $(CPPSRCS) $(CSRCS) $(SSRCS)
OBJS = $(CPPOBJS) $(COBJS) $(SOBJS)

CPPCOMPI=$(CPP) $(CFLAGS) -Wno-deprecated
CCCOMPI=$(CC) $(CFLAGS)

BUILDEXE = $(CPP) $(BFLAGS) -o $(BUILDDIR)/$@ $^ $(LINKS) ;
			
CLEAN = rm -f *.o ;\
		rm -rf build

# 在build文件夹下生成.o文件
CPPCOMPILE = $(CPPCOMPI) $< $(FLAGS) $(INCLS) $(MTOOL_INCL) -o $@
CCCOMPILE = $(CCCOMPI) $< $(FLAGS) $(INCLS) $(MTOOL_INCL) -o $@

# 链接静态库
ARSTATICLIB = $(AR) $@.tmp $^ $(AR_FLAGS); \
			  if [ $$? -ne 0 ]; then exit 1; fi; \
			  test -d $(STATICLIBPATH) || mkdir -p $(STATICLIBPATH); \
			  mv -f $@.tmp $(STATICLIBPATH)/$@;

# 链接动态库
BUILDSHARELIB = $(CPPSHARE) $@.tmp $^ $(BS_FLAGS); \
				if [ $$? -ne 0 ]; then exit 1; fi; \
				test -d $(DYNAMICLIBPATH) || mkdir -p $(DYNAMICLIBPATH); \
				mv -f $@.tmp $(DYNAMICLIBPATH)/$@;

$(BUILDDIR)/%.o : %.cpp
	mkdir -p build
	$(CPPCOMPILE)
$(BUILDDIR)/%.o : %.c
	mkdir -p build
	$(CCCOMPILE)
$(BUILDDIR)/%.o : %.S
	mkdir -p build
	$(CCCOMPILE)
$(BUILDDIR)/%.o : examples/%.cpp
	mkdir -p build
	$(CPPCOMPILE)
# $(CPPOBJS):%.o:%.cpp
# 	@echo $@

# $(CPPOBJS):$(CPPSRCS)
# 	mkdir -p build
# 	@echo "tttttttttttttt"
# 	$(CPPCOMPILE)
# $(CPPOBJS):build/%.o:%.cpp
# 	mkdir -p build
# 	@echo "tttttttttttttt"
# 	$(CPPCOMPILE)
# $(COBJS):build/%.o:%.c
# 	mkdir -p build
# 	$(CCCOMPILE)
# $(SOBJS):build/%.o:%.S
# 	mkdir -p build
# 	$(CCCOMPILE)

# %.o:%.cpp
# 	mkdir -p build
# 	$(CPPCOMPILE)
# .c.o:
# 	mkdir -p build
# 	$(CCCOMPILE)
# .S.o:
# 	mkdir -p build
# 	$(CCCOMPILE)
