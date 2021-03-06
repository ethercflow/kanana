#File: configure.ac
#Author: James Oakley
#Copyright (C): 2010 Dartmouth College
#License: GNU General Public License

AC_DEFUN([doMissingLibrary],[
  AC_MSG_ERROR([A required library is missing, abort])
])

AC_DEFUN([doMissingHeader],[
  AC_MSG_ERROR([A required header is missing, abort])
])

AC_DEFUN([libdwarfPrefixHeader],[
  AC_DEFINE([LIBDWARF_HEADER_NEEDS_PREFIX], ["1"], ["Use libdwarf/libdwarf.h instead of libdwarf.h"])
])

AC_DEFUN([libdwarfCheckHeaderNoPrefix],[
  AC_CHECK_HEADER(libdwarf.h, ,[doMissingHeader])
])



#we don't really want to do anything on successfully finding a library
#but autoconf is very strange and seems to automatically add the library to libs
#if we don't have some sort of action
AC_DEFUN([fndlibPlaceholder],[
  echo "required no-op" > /dev/null
])



AC_INIT([katana],[0.3.0b],[electron100@gmail.com])


AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([color-tests -Wall foreign])
SAVED_CFLAGS="$CFLAGS"
SAVED_CPPFLAGS="$CPPFLAGS"
SAVED_CXXFLAGS="$CXXFLAGS"
AC_PROG_CC
AC_PROG_CXX
CFLAGS=$SAVED_CFLAGS #avoid the default CFLAGS set by AC_PROG_CC
CPPFLAGS=$SAVED_CPPFLAGS #avoid the default set by AC_PROG_CXX
CXXFLAGS=$SAVED_CXXFLAGS #avoid the default set by AC_PROG_CXX
AM_PROG_CC_C_O
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile  src/Makefile tests/Makefile tests/code/Makefile tests/dwarf/Makefile tests/dwarf/passthrough/Makefile tests/dwarf/identity/Makefile src/shell/Makefile src/shell/dwarfscript/Makefile tests/hotpatch/Makefile tests/hotpatch/t1_basic/Makefile tests/hotpatch/t1_basic/v0/Makefile tests/hotpatch/t1_basic/v1/Makefile tests/hotpatch/t2_nested_types/Makefile tests/hotpatch/t2_nested_types/v0/Makefile tests/hotpatch/t2_nested_types/v1/Makefile tests/hotpatch/t3_multiple_cu/Makefile tests/hotpatch/t3_multiple_cu/v0/Makefile tests/hotpatch/t3_multiple_cu/v1/Makefile tests/hotpatch/t4_new_var/Makefile tests/hotpatch/t4_new_var/v0/Makefile tests/hotpatch/t4_new_var/v1/Makefile tests/hotpatch/t5_pointers/Makefile tests/hotpatch/t5_pointers/v0/Makefile tests/hotpatch/t5_pointers/v1/Makefile tests/hotpatch/t6_unions_enums/Makefile tests/hotpatch/t6_unions_enums/v0/Makefile tests/hotpatch/t6_unions_enums/v1/Makefile tests/hotpatch/t7_const/Makefile tests/hotpatch/t7_const/v0/Makefile tests/hotpatch/t7_const/v1/Makefile doc/Makefile])


#set up variables for use in Makefile.am
AC_CHECK_LIB(elf,elf_begin,[fndlibPlaceholder],[doMissingLibrary])
AC_CHECK_HEADER(dwarf.h, ,[doMissingHeader])
AC_CHECK_HEADER(libdwarf/libdwarf.h, [libdwarfPrefixHeader],[libdwarfCheckHeaderNoPrefix])
AC_CHECK_HEADER(libunwind.h, ,[doMissingHeader])
#todo: should check for libdwarf but I was having issues finding the correct symbol to do so
#AC_CHECK_LIB(dwarf,dwarf_dealloc,[fndlibPlaceholder],[doMissingLibrary])
#todo: should include check for libunwind-generic, but it seems to compile
#such that all its functions are prefixed differently on different platforms,
#so not sure what function to check for
#AC_CHECK_LIB(unwind-generic,[fndlibPlaceholder],[doMissingLibrary])
#todo: check both libunwinds we need, but I was having issues with these lines on some platforms
#AC_CHECK_LIB(unwind-ptrace,_UPT_create,[fndlibPlaceholder],[doMissingLibrary])
AC_CHECK_LIB(m,ceil,[fndlibPlaceholder],[doMissingLibrary])
AC_CHECK_LIB(readline,rl_initialize,[fndlibPlaceholder],[doMissingLibrary])

echo "operation system....."$host_os
case $host_os in
     linux*) SUPPORTED_OS=yes ;;
     *) echo "Your operating system is not supported by Katana. Please contact the Katana maintainers if you would like this fixed"; exit 1 ;;
esac

case $host_cpu in
     x86_64) SUPPORTED_CPU=yes ;;
     x86) SUPPORTED_CPU=yes ;;
     i686) SUPPORTED_CPU=yes ;;
     *) echo "Your CPU architecture is not supported by Katana. Please contact the Katana maintainers if you would like this fixed"; exit 1
esac


AC_CHECK_PROG(HAVE_PYTHON,python,yes,[no])
AC_CHECK_PROG(HAVE_BISON,bison,yes,[no])
AC_CHECK_PROG(HAVE_FLEX,flex,yes,[no])

LIBUNWIND=unwind-x86_64
thirtytwo_detected=`echo $host_cpu | grep i686``echo $CFLAGS | grep -e "-m32"`
if test -n "$thirtytwo_detected" ; then
   echo "Using 32-bit libunwind"
   LIBUNWIND=unwind-x86
fi
AC_SUBST([LIBUNWIND])


#todo: if don't have flex and bison, build everything except the shell
if test x$HAVE_BISON != xyes; then
   AC_MSG_ERROR(["No bison on this system, needed for katana shell"])
fi

if test x$HAVE_FLEX != xyes; then
   AC_MSG_ERROR(["No flex on this system, needed for katana shell"])
fi


AC_OUTPUT

#we put this message at the bottom so it is more likely to be seen
if test x$HAVE_PYTHON != xyes; then
   AC_MSG_WARN(["No python on this system. Unit tests will not work"])
fi
