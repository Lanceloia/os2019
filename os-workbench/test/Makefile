NAME := thread-os
SRCS := $(shell find -L . -name "*.c")
export TASK := TEST
export AM_HOME := $(PWD)/../abstract-machine
ifeq ($(ARCH),)
  export ARCH := x86-qemu
endif

include $(AM_HOME)/Makefile.app
include ../Makefile.lab
