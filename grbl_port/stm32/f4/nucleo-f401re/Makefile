##
## This file is part of the grbl_port_opencm3 project.
##
## Copyright (C) 2017 Angelo Di Chello
##
## Grbl_port_opencm3 is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## Grbl_port_opencm3 is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
## 
## You should have received a copy of the GNU General Public License
## along with Grbl_port_opencm3.  If not, see <http://www.gnu.org/licenses/>.
##

BINARY = main

LDSCRIPT = ./nucleo-f401re_ramcode.ld

CPPFLAGS += -I.  -I./cpu_map -I./defaults

OBJS		+= reset_vector.o nuts_bolts.o report.o stepper.o settings.o serial.o system.o print.o limits.o flash.o gcode.o motion_control.o protocol.o planner.o probe.o coolant_control.o spindle_control.o

include ../Makefile.include

LDLIBS += -lm 

