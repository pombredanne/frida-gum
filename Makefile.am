ACLOCAL_AMFLAGS = -I m4

libs =
extras =

if ENABLE_LIBS
libs += libs
endif

if ENABLE_BINDINGS
extras += bindings
endif

SUBDIRS = \
	$(libs) \
	gum \
	$(extras) \
	tests \
	vapi

pcfiles = \
	frida-gum-1.0.pc \
	frida-gumpp-1.0.pc

all-local: $(pcfiles)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = $(pcfiles)

if OS_ANDROID
RPFX = /data/local/tmp
MKDIR = mkdir
RM = rm
DEBUG_PORT = 5042

push: all
	adb shell "${MKDIR} \"${RPFX}/tests/\" && ${MKDIR} \"${RPFX}/tests/data/\" && ${RM} \"${RPFX}/tests/gum-tests\" && ${RM} \"${RPFX}/tests/data/targetfunctions-arm.so\" && ${RM} \"${RPFX}/tests/data/specialfunctions-arm.so\""
	adb push "$(top_builddir)/tests/gum-tests" "${RPFX}/tests/"
	adb push "$(top_srcdir)/tests/data/targetfunctions-arm.so" "${RPFX}/tests/data/"
	adb push "$(top_srcdir)/tests/data/specialfunctions-arm.so" "${RPFX}/tests/data/"

check: push
	adb shell "\"${RPFX}/tests/gum-tests\""

check-gdb: $(top_builddir)/tests/android-gdb.setup
	adb shell "${RM} -f /data/local/tmp/tests/debug-socket"
	adb push "${ANDROID_NDK_ROOT}/toolchains/arm-linux-androideabi-4.4.3/prebuilt/gdbserver" "${RPFX}/tests/"
	adb shell "${RPFX}/tests/gdbserver +${RPFX}/tests/debug-socket ${RPFX}/tests/gum-tests" &
	sleep 1
	adb forward tcp:${DEBUG_PORT} localfilesystem:${RPFX}/tests/debug-socket
	arm-linux-androideabi-gdb -x "$(top_builddir)/tests/android-gdb.setup"

pull:
	adb pull /system/bin/linker "$(top_builddir)/tests"
	adb pull /system/lib/libc.so "$(top_builddir)/tests"
	adb pull /system/lib/libstdc++.so "$(top_builddir)/tests"
	adb pull /system/lib/libm.so "$(top_builddir)/tests"
	adb pull /system/lib/libz.so "$(top_builddir)/tests"

$(top_builddir)/tests/android-gdb.setup: $(top_builddir)/tests/gum-tests
	echo "set solib-search-path $(abs_top_builddir)/tests:$(abs_top_srcdir)/tests/data:${ANDROID_SYSROOT}/usr/lib" > $@
	echo "directory ${ANDROID_SYSROOT}/usr/include" >> $@
	echo "directory $(abs_top_srcdir)/gum" >> $@
	echo "file $(abs_top_builddir)/tests/gum-tests" >> $@
	echo "target remote :${DEBUG_PORT}" >> $@
endif

if OS_IOS
RURL = root@${IOS_IPADDR}
RPFX = /Library/Frida
RSYNC = rsync -az

check: all
	ssh "${RURL}" "mkdir -p \"${RPFX}/tests/data/\""
	${RSYNC} \
		"$(top_builddir)/tests/gum-tests" \
		"${RURL}:\"${RPFX}/tests/\""
	${RSYNC} \
		"$(top_srcdir)/tests/data/targetfunctions-arm.dylib" \
		"$(top_srcdir)/tests/data/specialfunctions-arm.dylib" \
		"${RURL}:\"${RPFX}/tests/data/\""
	ssh "${RURL}" "\"${RPFX}/tests/gum-tests\""
endif
