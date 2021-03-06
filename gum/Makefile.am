lib_LTLIBRARIES = libfrida-gum-1.0.la
noinst_LTLIBRARIES = libfrida-gum.la

fridaincludedir = $(includedir)/frida-1.0/gum
fridainclude_HEADERS = \
	gum.h \
	gumarray.h \
	gumbacktracer.h \
	gumclosure.h \
	gumcodeallocator.h \
	gumdefs.h \
	gumevent.h \
	gumeventsink.h \
	gumfunction.h \
	gumhash.h \
	guminterceptor.h \
	guminvocationcontext.h \
	guminvocationlistener.h \
	gumlist.h \
	gummemory.h \
	gummemoryaccessmonitor.h \
	gummemorymap.h \
	gumprocess.h \
	gumreturnaddress.h \
	gumspinlock.h \
	gumstalker.h \
	gumsymbolutil.h \
	gumsysinternals.h

libfrida_gum_1_0_la_SOURCES = $(NULL)
libfrida_gum_1_0_la_LDFLAGS = \
	-export-symbols-regex "^gum_.*" \
	$(GUM_LDFLAGS)
libfrida_gum_1_0_la_LIBADD = \
	libfrida-gum.la \
	$(GUM_LIBS)
libfrida_gum_1_0_la_LIBTOOLFLAGS = \
	--tag=disable-shared

arch_sources = $(NULL)
arch_includes = $(NULL)
arch_libs = $(NULL)
backend_sources = $(NULL)
script_sources = $(NULL)
libs_libadd = $(NULL)

if ARCH_I386
archincludedir = $(includedir)/frida-1.0/gum/arch-x86
archinclude_HEADERS = \
	arch-x86/gumx86backtracer.h \
	arch-x86/gumx86writer.h \
	arch-x86/gumx86relocator.h \
	arch-x86/gumx86reader.h \
	arch-x86/gumx86functionparser.h
arch_sources += \
	arch-x86/gumx86backtracer.c \
	arch-x86/gumx86writer.c \
	arch-x86/gumx86relocator.c \
	arch-x86/gumx86reader.c \
	arch-x86/gumx86functionparser.c \
	arch-x86/gumudis86.h \
	arch-x86/gumudis86.c
arch_includes += \
	-I $(srcdir)/arch-x86
backend_sources += \
	backend-x86/gumclosure-x86.c \
	backend-x86/guminterceptor-x86.c \
	backend-x86/gumspinlock-x86.c \
	backend-x86/gumstalker-x86.c \
	backend-x86/gumstalker-x86-glue.S
endif

if ARCH_ARM
archincludedir = $(includedir)/frida-1.0/gum/arch-arm
archinclude_HEADERS = \
	arch-arm/gumarm.h \
	arch-arm/gumarmbacktracer.h \
	arch-arm/gumarmwriter.h \
	arch-arm/gumarmrelocator.h \
	arch-arm/gumthumbwriter.h \
	arch-arm/gumthumbrelocator.h
arch_sources += \
	arch-arm/gumarmbacktracer.c \
	arch-arm/gumarmwriter.c \
	arch-arm/gumarmrelocator.c \
	arch-arm/gumthumbwriter.c \
	arch-arm/gumthumbrelocator.c
arch_includes += \
	-I $(srcdir)/arch-arm
backend_sources += \
	backend-arm/gumclosure-arm.c \
	backend-arm/guminterceptor-arm.c \
	backend-arm/gumspinlock-arm.c \
	backend-arm/gumstalker-arm.c
endif

if OS_LINUX
backend_sources += \
	backend-linux/gumprocess-linux.c
fridainclude_HEADERS += \
	backend-linux/gumlinux.h
endif

if OS_DARWIN
if ENABLE_OBJC
backend_sources += \
	backend-darwin/gumnsbacktracer.m \
	backend-darwin/gumsymbolutil-darwin.m
endif
backend_sources += \
	backend-darwin/gummemory-darwin.c \
	backend-darwin/gumprocess-darwin.c
fridainclude_HEADERS += \
	backend-darwin/gumdarwin.h \
	backend-darwin/gumnsbacktracer.h
else
backend_sources += \
	backend-unix/gummemory-unix.c
endif

if HAVE_GLIBC
backend_sources += \
	backend-glibc/gumgnubacktracer.c
endif

if HAVE_BFD
if ENABLE_OBJC
else
backend_sources += \
	backend-bfd/gumsymbolutil-bfd.c
endif
endif

if OS_ANDROID
else
backend_sources += backend-gcc/gumgccbacktracer.c
fridainclude_HEADERS += backend-gcc/gumgccbacktracer.h
endif

if HAVE_V8
script_sources += \
	gumscript.cpp \
	gumscript-priv.h \
	gumscript-runtime.h \
	gumscriptscope.h \
	gumscriptcore.h \
	gumscriptcore.cpp \
	gumscriptmemory.h \
	gumscriptmemory.cpp \
	gumscriptprocess.h \
	gumscriptprocess.cpp \
	gumscriptthread.h \
	gumscriptthread.cpp \
	gumscriptmodule.h \
	gumscriptmodule.cpp \
	gumscriptfile.h \
	gumscriptfile.cpp \
	gumscriptsocket.h \
	gumscriptsocket.cpp \
	gumscriptinterceptor.h \
	gumscriptinterceptor.cpp \
	gumscriptstalker.h \
	gumscriptstalker.cpp \
	gumscripteventsink.h \
	gumscripteventsink.cpp
fridainclude_HEADERS += \
	gumscript.h
endif

if ENABLE_LIBS
libs_libadd += \
	$(top_builddir)/libs/gum/heap/libfrida-gum-heap-1.0.la \
	$(top_builddir)/libs/gum/prof/libfrida-gum-prof-1.0.la
endif

container_sources = \
	gumarray.c \
	gumhash.c \
	gumlist.c

libfrida_gum_la_SOURCES = \
	gum.c \
	gumbacktracer.c \
	gumcodeallocator.c \
	gumeventsink.c \
	guminterceptor.c \
	guminvocationcontext.c \
	guminvocationlistener.c \
	gummemory.c \
	gummemorymap.c \
	gumreturnaddress.c \
	gumtls.h \
	$(arch_sources) \
	$(container_sources) \
	$(script_sources) \
	$(backend_sources)
libfrida_gum_la_LIBADD = \
	$(libs_libadd) \
	$(arch_libs)
libfrida_gum_la_LIBTOOLFLAGS = \
	--tag=disable-shared

AM_CPPFLAGS = \
	-include config.h \
	-I $(top_srcdir) \
	$(arch_includes) \
	$(GUM_CFLAGS)
