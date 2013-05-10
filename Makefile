#    This file is part of AutoQuad.
#
#    AutoQuad is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    AutoQuad is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#    You should have received a copy of the GNU General Public License
#    along with AutoQuad.  If not, see <http://www.gnu.org/licenses/>.
#
#    Copyright © 2011, 2012  Bill Nesbitt

CC = g++
CFLAGS = -g -O3
LDFLAGS =

ALL_CFLAGS = $(CFLAGS)

# BUILD_PATH = .
BUILD_PATH = ../build

# Linux/OS X
#LIBPATH = /opt/local/lib
#INCPATH = /opt/local/include
#MAVLINK = ../mavlink/include/common
#EXPAT_LIB = expat
#PLPLOT_LIB = plplotd
#EIGEN = /usr/local/include/eigen3

# Windows
LIBPATH = ../../../lib
INCPATH = .
MAVLINK = $(LIBPATH)/mavlink/include/common
EXPAT = $(LIBPATH)/expat
EXPAT_LIB = libexpat
EIGEN = $(LIBPATH)/eigen


# Targets

all: loader telemetryDump logDump batCal l1Tool

all-win: logDump-win l1Tool-win

loader: $(BUILD_PATH)/loader.o $(BUILD_PATH)/serial.o $(BUILD_PATH)/stmbootloader.o
	$(CC) -o $(BUILD_PATH)/loader $(ALL_CFLAGS) $(BUILD_PATH)/loader.o $(BUILD_PATH)/serial.o $(BUILD_PATH)/stmbootloader.o

telemetryDump: $(BUILD_PATH)/telemetryDump.o $(BUILD_PATH)/serial.o
	$(CC) -o $(BUILD_PATH)/telemetryDump $(ALL_CFLAGS) $(BUILD_PATH)/telemetryDump.o $(BUILD_PATH)/serial.o

logDump: $(BUILD_PATH)/logDump.o $(BUILD_PATH)/serial.o $(BUILD_PATH)/logger.o
	$(CC) -o $(BUILD_PATH)/logDump $(ALL_CFLAGS) $(BUILD_PATH)/logDump.o $(BUILD_PATH)/serial.o $(BUILD_PATH)/logger.o -L$(LIBPATH) -l$(PLPLOT_LIB) -DHAS_PLPLOT

logDump-win: $(BUILD_PATH)/logDump-win.o $(BUILD_PATH)/logger.o $(BUILD_PATH)/logDump_mavlink.o
	$(CC) -o $(BUILD_PATH)/logDump.exe $(ALL_CFLAGS) $(BUILD_PATH)/logDump-win.o $(BUILD_PATH)/logger.o $(BUILD_PATH)/logDump_mavlink.o

batCal: $(BUILD_PATH)/batCal.o $(BUILD_PATH)/logger.o
	$(CC) -o batCal $(ALL_CFLAGS) $(BUILD_PATH)/batCal.o $(BUILD_PATH)/logger.o -L$(LIBPATH) -l$(PLPLOT_LIB)

l1Tool: $(BUILD_PATH)/l1Tool.o
	$(CC) -o $(BUILD_PATH)/l1Tool $(ALL_CFLAGS) $(BUILD_PATH)/l1Tool.o -l$(EXPAT_LIB)

l1Tool-win: $(BUILD_PATH)/l1Tool-win.o
	$(CC) -o $(BUILD_PATH)/l1Tool.exe $(ALL_CFLAGS) $(BUILD_PATH)/l1Tool.o -L$(EXPAT) -l$(EXPAT_LIB)



$(BUILD_PATH)/loader.o: loader.c serial.h stmbootloader.h
	$(CC) -c $(ALL_CFLAGS) loader.c -o $@

$(BUILD_PATH)/stmbootloader.o: stmbootloader.c stmbootloader.h serial.o
	$(CC) -c $(ALL_CFLAGS) stmbootloader.c -o $@

$(BUILD_PATH)/serial.o: serial.c serial.h
	$(CC) -c $(ALL_CFLAGS) serial.c -o $@

$(BUILD_PATH)/telemetryDump.o: telemetryDump.c telemetryDump.h
	$(CC) -c $(ALL_CFLAGS) telemetryDump.c -o $@

$(BUILD_PATH)/logDump.o: logDump.c logDump_templates.h logger.h
	$(CC) -c $(ALL_CFLAGS) logDump.c -o $@ -I$(INCPATH) -DHAS_PLPLOT

$(BUILD_PATH)/logDump-win.o: logDump.c logDump_templates.h logDump.h logger.h logDump_mavlink.h
	$(CC) -c $(ALL_CFLAGS) logDump.c -o $@ -I$(MAVLINK)

$(BUILD_PATH)/batCal.o: batCal.cc
	$(CC) -c $(ALL_CFLAGS) batCal.cc -o $@ -I$(INCPATH) -I$(EIGEN)

$(BUILD_PATH)/l1Tool.o: l1Tool.cc
	$(CC) -c $(ALL_CFLAGS) l1Tool.cc -o $@ -I$(EIGEN)

$(BUILD_PATH)/l1Tool-win.o: l1Tool.cc
	$(CC) -c -g -O2 l1Tool.cc -o $(BUILD_PATH)/l1Tool.o -I$(EXPAT)/src -I$(EIGEN)

$(BUILD_PATH)/logger.o: logger.c logger.h
	$(CC) -c $(ALL_CFLAGS) logger.c -o $@

$(BUILD_PATH)/logDump_mavlink.o: logDump_mavlink.cpp logDump_mavlink.h
	$(CC) -c $(ALL_CFLAGS) logDump_mavlink.cpp -o $@ -I$(MAVLINK)

clean:
	rm -f $(BUILD_PATH)/loader $(BUILD_PATH)/telemetryDump $(BUILD_PATH)/logDump $(BUILD_PATH)/batCal $(BUILD_PATH)/l1Tool $(BUILD_PATH)/*.o $(BUILD_PATH)/*.exe
