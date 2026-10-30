include vendor/NickelHook/NickelHook.mk
override LIBRARY := libdailysync.so
override VERSION := $(shell cat VERSION)
override SOURCES += src/daily.cc
override MOCS += src/daily.h
override CFLAGS += -Wall -Wextra -Werror -Wno-missing-field-initializers -fvisibility=hidden
override CXXFLAGS += -Wall -Wextra -Werror -Wno-missing-field-initializers -fvisibility=hidden -fvisibility-inlines-hidden
override PKGCONF += Qt5Widgets
override KOBOROOT += resources/installed:/mnt/onboard/.adds/daily-sync/installed
override KOBOROOT += resources/paused:/mnt/onboard/.adds/daily-sync/paused
include vendor/NickelHook/NickelHook.mk

koboroot: $(LIBRARY)
# NickelHook passes all prerequisites to this helper; compile source files only.
override nh_cmd_cc = $(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(filter %.cc %.cpp,$(2)) -o $(1)
src/daily.o: src/daily.h src/policy.h src/storage.h
$(OBJECTS_C) $(OBJECTS_CXX) $(OBJECTS_CXX1) $(OBJECTS_MOC): VERSION
override nh_cmd_c = $(CC) $(CPPFLAGS) $(CFLAGS) -c $(filter %.c,$(2)) -o $(1)
override nh_cmd_moco = $(CXX) -xc++ $(CPPFLAGS) $(CXXFLAGS) -c $(filter %.moc,$(2)) -o $(1)
