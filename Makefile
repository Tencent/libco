#
# Tencent is pleased to support the open source community by making Libco available.
# 
# Copyright (C) 2014 THL A29 Limited, a Tencent company. All rights reserved.
# 
# Licensed under the Apache License, Version 2.0 (the "License"); 
# you may not use this file except in compliance with the License. 
# You may obtain a copy of the License at
# 
#   http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, 
# software distributed under the License is distributed on an "AS IS" BASIS, 
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
# See the License for the specific language governing permissions and 
# limitations under the License.
#


COMM_MAKE = 1
COMM_ECHO = 1
version=0.5
v=debug
include co.mk

########## options ##########
CFLAGS += -g -fno-strict-aliasing -O2 -Wall -export-dynamic \
	-Wall -pipe  -D_GNU_SOURCE -D_REENTRANT -fPIC -Wno-deprecated -m64

UNAME := $(shell uname -s)

ifeq ($(UNAME), FreeBSD)
LINKS += -g -L./lib -lcolib -lpthread
else
LINKS += -g -L./lib -lcolib -lpthread -ldl
endif

# COLIB_OBJS  = $(wildcard *.c)
COLIB_OBJS  = $(OBJS)
# COLIB_OBJS  = $(patsubst %.o,$(BUILDDIR)/%.o,$(OBJS))
# COLIB_OBJS=co_epoll.o co_routine.o co_hook_sys_call.o coctx_swap.o coctx.o co_comm.o
# COLIB_OBJS=$(BUILDDIR)/co_epoll.o $(BUILDDIR)/co_routine.o $(BUILDDIR)/co_hook_sys_call.o $(BUILDDIR)/coctx_swap.o $(BUILDDIR)/coctx.o $(BUILDDIR)/co_comm.o

#co_swapcontext.o

PROGS = colib example_poll example_echosvr example_echocli example_thread  example_cond example_specific example_copystack example_setenv

all:$(PROGS)
	rm -f build/*.o

colib:libcolib.a libcolib.so

libcolib.a: $(COLIB_OBJS)
	$(ARSTATICLIB) 
libcolib.so: $(COLIB_OBJS)
	$(BUILDSHARELIB) 

example_echosvr:$(BUILDDIR)/example_echosvr.o
	$(BUILDEXE) 
example_echocli:$(BUILDDIR)/example_echocli.o
	$(BUILDEXE) 
example_thread:$(BUILDDIR)/example_thread.o
	$(BUILDEXE) 
example_poll:$(BUILDDIR)/example_poll.o
	$(BUILDEXE) 
example_exit:$(BUILDDIR)/example_exit.o
	$(BUILDEXE) 
example_cond:$(BUILDDIR)/example_cond.o
	$(BUILDEXE)
example_specific:$(BUILDDIR)/example_specific.o
	$(BUILDEXE)
example_copystack:$(BUILDDIR)/example_copystack.o
	$(BUILDEXE)
example_setenv:$(BUILDDIR)/example_setenv.o
	$(BUILDEXE)

dist: clean libco-$(version).src.tar.gz

libco-$(version).src.tar.gz:
	@find . -type f | grep -v CVS | grep -v .svn | sed s:^./:libco-$(version)/: > MANIFEST
	@(cd ..; ln -s libco_pub libco-$(version))
	(cd ..; tar cvf - `cat libco_pub/MANIFEST` | gzip > libco_pub/libco-$(version).src.tar.gz)
	@(cd ..; rm libco-$(version))

clean:
	$(CLEAN) *.o $(PROGS)
	rm -fr MANIFEST lib solib libco-$(version).src.tar.gz libco-$(version)

