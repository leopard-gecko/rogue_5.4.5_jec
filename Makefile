# Select the native Windows build automatically when using MSYS2 UCRT64.
# The existing Unix Makefile below remains unchanged for macOS and Linux.
# ROGUE_BUILD=windows or ROGUE_BUILD=unix overrides automatic selection.
ifeq ($(strip $(ROGUE_BUILD)),)
ifeq ($(MSYSTEM),UCRT64)
ROGUE_BUILD := windows
else ifeq ($(OS),Windows_NT)
# Windows-native make started outside MSYS2 has no MSYSTEM variable.
# Do not select the Windows-native build inside the MSYS/Cygwin-like shell.
ifeq ($(strip $(MSYSTEM)),)
ROGUE_BUILD := windows
else
ROGUE_BUILD := unix
endif
else
ROGUE_BUILD := unix
endif
endif

ifeq ($(ROGUE_BUILD),windows)
include Makefile.win
else ifeq ($(ROGUE_BUILD),unix)
###############################################################################
#
# Makefile for rogue
#
# Rogue: Exploring the Dungeons of Doom
# Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
# All rights reserved.
#       
# See the file LICENSE.TXT for full copyright and licensing information.
#
###############################################################################
        
###############################################################################
# Site configuration occurs beneath this comment.
# The checked-in Makefile can be used directly.  The optional ./configure
# helper writes config.mk, which is included below when present.
# Override variables on the make command line when local adjustments are needed.
###############################################################################

DISTNAME = rogue-5.4.5jec.056
PROG ?=

-include config.mk

PROGRAM ?= $(if $(PROG),$(PROG),rogue)
FILEDIR ?=

# GNU make does not expand a leading ~ in a variable value.  Resolve it
# for direct make FILEDIR=~/... and explicit SAVEFILE/SCOREFILE overrides.
# configure already stores expanded paths, so these are complementary.
ifneq ($(filter ~ ~/%,$(FILEDIR) $(SAVEFILE) $(SCOREFILE)),)
ifeq ($(strip $(HOME)),)
$(error HOME is required for paths beginning with ~)
endif
endif
expand_home = $(if $(filter ~,$(1)),$(HOME),$(patsubst ~/%,$(HOME)/%,$(1)))
RESOLVED_FILEDIR := $(call expand_home,$(FILEDIR))

ifneq ($(strip $(RESOLVED_FILEDIR)),)
SAVEFILE ?= $(RESOLVED_FILEDIR)/$(PROGRAM).save
SCOREFILE ?= $(RESOLVED_FILEDIR)/$(PROGRAM).scr
else
SAVEFILE ?= $(PROGRAM).save
SCOREFILE ?= $(PROGRAM).scr
endif
RESOLVED_SAVEFILE := $(call expand_home,$(SAVEFILE))
RESOLVED_SCOREFILE := $(call expand_home,$(SCOREFILE))
MANPAGE_EN ?= build/man/en/$(PROGRAM).6
MANPAGE_JA ?= build/man/ja/$(PROGRAM).6
MANPAGES = $(MANPAGE_EN) $(MANPAGE_JA)
MANPAGE_EN_TEMPLATE ?= rogue.6.en.in
MANPAGE_JA_TEMPLATE ?= rogue.6.ja.in

O = o

CC ?= cc
CFLAGS ?= -g -O2
CPPFLAGS ?=
DEFAULT_OPTION_CPPFLAGS ?= -DMODERN_ROGUE_OPTIONS
DEFAULT_LANGUAGE ?= 0
ifneq ($(strip $(DEFAULT_LANGUAGE)),)
LANGUAGE_CPPFLAGS = -DDEFAULT_LANGUAGE=$(DEFAULT_LANGUAGE)
endif
LDFLAGS ?=

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
CURSES_CFLAGS ?=
CURSES_LIBS ?= -lcurses
else
CURSES_CFLAGS ?= $(shell pkg-config --cflags ncursesw 2>/dev/null)
CURSES_LIBS ?= $(shell pkg-config --libs ncursesw 2>/dev/null)
ifeq ($(strip $(CURSES_LIBS)),)
CURSES_LIBS = -lncursesw
endif
endif
LIBS ?= $(CURSES_LIBS)

RM ?= rm -f


# Default save and score files follow PROGRAM unless overridden.
# Set FILEDIR to place both files in the same directory.
# Individual SAVEFILE and SCOREFILE settings still take precedence.
# PROG is accepted as a short alias used by the other Rogue ports.
# If both PROGRAM and PROG are specified, PROGRAM takes precedence.
# For example, make PROGRAM=jrogue or make PROG=jrogue builds jrogue
# and uses jrogue.save and jrogue.scr by default.
# make PROGRAM=jrogue FILEDIR=/var/games/rogue uses
# /var/games/rogue/jrogue.save and jrogue.scr.

# Keep the built-in flags even when the user passes CPPFLAGS on the
# command line.  Use EXTRA_CPPFLAGS for local additions such as -DMASTER.
USER_CPPFLAGS := $(CPPFLAGS)
# Quote the complete -D argument, including paths with spaces in HOME.
shell_quote = '$(subst ','"'"',$(1))'
# Escape sed replacement metacharacters before quoting the whole expression.
sed_replacement = $(subst |,\|,$(subst &,\&,$(subst \,\\,$(1))))
CPP_SAVEFILE := $(call shell_quote,"$(RESOLVED_SAVEFILE)")
CPP_SCOREFILE := $(call shell_quote,"$(RESOLVED_SCOREFILE)")
override CPPFLAGS := -DHAVE_CONFIG_H \
                     -DDEFAULT_SAVEFILE=$(CPP_SAVEFILE) \
                     -DSCOREFILE=$(CPP_SCOREFILE) \
                     $(DEFAULT_OPTION_CPPFLAGS) \
                     $(LANGUAGE_CPPFLAGS) \
                     -I. $(CURSES_CFLAGS) $(USER_CPPFLAGS) $(EXTRA_CPPFLAGS)
ifeq ($(MASTER),1)
override CPPFLAGS += -DMASTER
endif

DISTFILE = $(DISTNAME)

INSTALL ?= install
MKDIR ?= mkdir -p
RMDIR ?= rmdir

DESTDIR ?=

prefix ?= /usr/local
exec_prefix ?= $(prefix)
datarootdir ?= $(prefix)/share
datadir ?= $(datarootdir)
bindir ?= $(exec_prefix)/bin
mandir ?= $(datarootdir)/man
docdir ?= $(datarootdir)/doc/$(PROGRAM)
man6dir ?= $(mandir)/man6
# By default, install Japanese manual pages in the common Linux location
# and in the locale-specific locations used by macOS's standard man(1).
# ./configure may replace this with a platform-specific subset.
JAMAN6DIRS ?= $(mandir)/ja/man6 $(mandir)/ja_JP.UTF-8/man6 $(mandir)/ja.UTF-8/man6

###############################################################################
# Site configuration occurs above this comment
# It should not be necessary to change anything below this comment
############################################################################### 

HDRS     = rogue.h extern.h score.h jec/color.h jec/language.h jec/unicode_text.h
OBJS1    = jec/color.$(O) jec/unicode_text.$(O) vers.$(O) extern.$(O) armor.$(O) chase.$(O) command.$(O) \
           daemon.$(O) daemons.$(O) fight.$(O) init.$(O) io.$(O) list.$(O) jec/language.$(O) \
           mach_dep.$(O) main.$(O) mdport.$(O) misc.$(O) monsters.$(O) \
           move.$(O) new_level.$(O)
OBJS2    = options.$(O) pack.$(O) passages.$(O) potions.$(O) rings.$(O) \
           rip.$(O) rooms.$(O) save.$(O) scrolls.$(O) state.$(O) sticks.$(O) \
           things.$(O) weapons.$(O) wizard.$(O) xcrypt.$(O)
OBJS     = $(OBJS1) $(OBJS2)
CFILES   = jec/color.c jec/unicode_text.c vers.c extern.c armor.c chase.c command.c daemon.c \
           daemons.c fight.c init.c io.c list.c jec/language.c mach_dep.c \
           main.c  mdport.c misc.c monsters.c move.c new_level.c \
           options.c pack.c passages.c potions.c rings.c rip.c \
           rooms.c save.c scrolls.c state.c sticks.c things.c \
           weapons.c wizard.c xcrypt.c windows/rogue_windows.c windows/rogue_windows_console.c
README_FILES = README_mac_linux_ja.md README_mac_linux_en.md \
               README_windows_ja.md README_windows_en.md
DOCS     = $(README_FILES) rogue_ja.txt rogue_en.txt \
           rogue_ja.html rogue_en.html $(MANPAGES)
AFILES   = Makefile Makefile.win windows/rogue_windows.h windows/rogue_windows_console.h configure config.h \
           rogue.6.en.in rogue.6.ja.in README.md $(README_FILES) \
           rogue_ja.txt rogue_en.txt rogue_ja.html rogue_en.html \
           windows/rogue.ico windows/rogue-icon.rc windows/windows-saved-games.ps1 \
           windows/README_ja.txt windows/README_en.txt
MISC     = LICENSE.TXT
.PHONY: all japanese english original master wizard clean distclean stddocs dist dist.src install uninstall reinstall

all: $(PROGRAM) $(MANPAGES)

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# Recompile score users when the score interface changes.
save.$(O) rip.$(O): score.h
    
japanese:
	$(MAKE) clean
	$(MAKE) all DEFAULT_LANGUAGE=1 EXTRA_CPPFLAGS="$(EXTRA_CPPFLAGS)"

english:
	$(MAKE) clean
	$(MAKE) all DEFAULT_LANGUAGE=2 EXTRA_CPPFLAGS="$(EXTRA_CPPFLAGS)"

# Original target: English by default, with Rogue-style option defaults.
original:
	$(MAKE) clean
	$(MAKE) all DEFAULT_OPTION_CPPFLAGS= DEFAULT_LANGUAGE=$(if $(filter command line,$(origin DEFAULT_LANGUAGE)),$(DEFAULT_LANGUAGE),2) EXTRA_CPPFLAGS="$(EXTRA_CPPFLAGS)"

master:
	$(MAKE) clean
	$(MAKE) all MASTER=1

# Backward-compatible target name; the compile-time macro is still MASTER.
wizard: master

$(PROGRAM): $(HDRS) $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS) -o $@

$(MANPAGE_EN): $(MANPAGE_EN_TEMPLATE)
	$(MKDIR) $(dir $@)
	sed -e $(call shell_quote,s|@PROGRAM@|$(PROGRAM)|g) \
	    -e $(call shell_quote,s|@SAVEFILE@|$(call sed_replacement,$(RESOLVED_SAVEFILE))|g) \
	    -e $(call shell_quote,s|@SCOREFILE@|$(call sed_replacement,$(RESOLVED_SCOREFILE))|g) \
	    $(MANPAGE_EN_TEMPLATE) > $@

$(MANPAGE_JA): $(MANPAGE_JA_TEMPLATE)
	$(MKDIR) $(dir $@)
	sed -e $(call shell_quote,s|@PROGRAM@|$(PROGRAM)|g) \
	    -e $(call shell_quote,s|@SAVEFILE@|$(call sed_replacement,$(RESOLVED_SAVEFILE))|g) \
	    -e $(call shell_quote,s|@SCOREFILE@|$(call sed_replacement,$(RESOLVED_SCOREFILE))|g) \
	    $(MANPAGE_JA_TEMPLATE) > $@
    
# Game data (save and score files) must survive clean/distclean.
clean:
	$(RM) $(OBJS1)
	$(RM) $(OBJS2)
	$(RM) core a.exe a.out a.exe.stackdump $(PROGRAM) $(PROGRAM).exe $(PROG) $(PROG).exe rogue rogue.exe
	$(RM) $(PROGRAM).tar $(PROGRAM).tar.gz $(PROGRAM).zip $(PROG).tar $(PROG).tar.gz $(PROG).zip rogue.tar rogue.tar.gz rogue.zip
	$(RM) $(MANPAGES) $(PROGRAM).6 rogue.6
	$(RM) -r build/man
	@$(RMDIR) build 2>/dev/null || true

# Remove files generated by ./configure.  clean intentionally keeps config.mk.
distclean: clean
	$(RM) config.mk config.log
    

stddocs: $(MANPAGES)
	@echo "$(MANPAGE_EN) generated from $(MANPAGE_EN_TEMPLATE)."
	@echo "$(MANPAGE_JA) generated from $(MANPAGE_JA_TEMPLATE)."

# Package the explicitly listed source files without cleaning the working tree.
# Keep this as the one canonical manifest for both Unix and Windows builds.
dist.src:
	@set -e; \
	  stage=$$(mktemp -d); \
	  trap 'rm -rf "$$stage"' 0; \
	  mkdir "$$stage/$(DISTNAME)"; \
	  for file in $(CFILES) $(HDRS) $(MISC) $(AFILES); do \
	    dir=$${file%/*}; \
	    if [ "$$dir" != "$$file" ]; then mkdir -p "$$stage/$(DISTNAME)/$$dir"; fi; \
	    COPYFILE_DISABLE=1 cp "$$file" "$$stage/$(DISTNAME)/$$file"; \
	  done; \
	  chmod 755 "$$stage/$(DISTNAME)/configure"; \
	  COPYFILE_DISABLE=1 tar --no-xattrs -C "$$stage" -cf "$(DISTNAME)-src.tar" "$(DISTNAME)"; \
	  gzip -f "$(DISTNAME)-src.tar"; \
	  printf 'Created source archive: %s\n' "$(CURDIR)/$(DISTNAME)-src.tar.gz"

dist: clean $(PROGRAM) $(MANPAGES)
	tar cf $(DISTFILE).tar $(PROGRAM) $(MISC) $(DOCS)
	gzip -f $(DISTFILE).tar

install: $(PROGRAM) $(MANPAGES)
	$(MKDIR) $(DESTDIR)$(bindir)
	$(MKDIR) $(DESTDIR)$(man6dir)
	$(MKDIR) $(DESTDIR)$(docdir)
ifneq ($(strip $(RESOLVED_FILEDIR)),)
	$(MKDIR) "$(DESTDIR)$(RESOLVED_FILEDIR)"
endif
	$(INSTALL) -m 0755 $(PROGRAM) $(DESTDIR)$(bindir)/$(PROGRAM)
	$(INSTALL) -m 0644 $(MANPAGE_EN) $(DESTDIR)$(man6dir)/$(PROGRAM).6
	@for d in $(JAMAN6DIRS); do \
	    $(MKDIR) $(DESTDIR)$$d; \
	    $(INSTALL) -m 0644 $(MANPAGE_JA) $(DESTDIR)$$d/$(PROGRAM).6; \
	done
	@for f in $(README_FILES); do \
	    $(INSTALL) -m 0644 $$f $(DESTDIR)$(docdir)/$$f; \
	done
	$(INSTALL) -m 0644 rogue_ja.txt $(DESTDIR)$(docdir)/rogue_ja.txt
	$(INSTALL) -m 0644 rogue_en.txt $(DESTDIR)$(docdir)/rogue_en.txt
	$(INSTALL) -m 0644 rogue_ja.html $(DESTDIR)$(docdir)/rogue_ja.html
	$(INSTALL) -m 0644 rogue_en.html $(DESTDIR)$(docdir)/rogue_en.html
	$(INSTALL) -m 0644 LICENSE.TXT $(DESTDIR)$(docdir)/LICENSE.TXT

uninstall:	
	-$(RM) $(DESTDIR)$(bindir)/$(PROGRAM)
	-$(RM) $(DESTDIR)$(man6dir)/$(PROGRAM).6
	@for d in $(JAMAN6DIRS); do \
	    $(RM) $(DESTDIR)$$d/$(PROGRAM).6; \
	    $(RMDIR) $(DESTDIR)$$d 2>/dev/null || true; \
	    parent=$${d%/man6}; \
	    if [ "$$parent" != "$$d" ]; then \
	        $(RMDIR) $(DESTDIR)$$parent 2>/dev/null || true; \
	    fi; \
	done
	@for f in $(README_FILES); do $(RM) $(DESTDIR)$(docdir)/$$f; done
	-$(RM) $(DESTDIR)$(docdir)/rogue_ja.txt
	-$(RM) $(DESTDIR)$(docdir)/rogue_en.txt
	-$(RM) $(DESTDIR)$(docdir)/rogue_ja.html
	-$(RM) $(DESTDIR)$(docdir)/rogue_en.html
	-$(RM) $(DESTDIR)$(docdir)/LICENSE.TXT
	-$(RMDIR) $(DESTDIR)$(docdir)
    
reinstall: uninstall install

else
$(error Unsupported ROGUE_BUILD=$(ROGUE_BUILD); use windows or unix)
endif
