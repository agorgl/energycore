#=- Makefile -=#
# CreateProcess NULL bug
ifeq ($(OS), Windows_NT)
	SHELL = cmd.exe
endif

# Set current makefile location
MKLOC ?= $(CURDIR)/$(firstword $(MAKEFILE_LIST))
export MKLOC

#---------------------------------------------------------------
# Usage
#---------------------------------------------------------------
# The following generic Makefile is based on a specific project
# structure that makes the following conventions:
#
# The vanilla directory structure is in the form:
# - Root
#   - include [1]
#   - deps    [2]
#   - src     [3]
# Where:
#  [1] exists only for the library projects, and contains the header files
#  [2] exists only for the projects with external dependencies,
#      and each subfolder represents a dependency with a project
#      in the form we are currently documenting
#  [3] contains the source code for the project
#
# A working (built) project can also contain the following
# - Root
#   - bin [5]
#   - lib [6]
#   - tmp [7]
# Where:
#  [5] contains binary results of the linking process (executables, dynamic libraries)
#  [6] contains archive results of the archiving process (static libraries)
#  [7] contains intermediate object files of compiling processes

#---------------------------------------------------------------
# Build variable parameters
#---------------------------------------------------------------
# Variant = (Debug|Release)
VARIANT ?= Debug
SILENT ?=
TOOLCHAIN ?= GCC

#---------------------------------------------------------------
# Helpers
#---------------------------------------------------------------
# Recursive wildcard func
rwildcard = $(foreach d, $(wildcard $1*), $(call rwildcard, $d/, $2) $(filter $(subst *, %, $2), $d))

# Suppress command errors
ifeq ($(OS), Windows_NT)
	suppress_out = > nul 2>&1 || (exit 0)
else
	suppress_out = > /dev/null 2>&1 || true
endif

# Native paths
ifeq ($(OS), Windows_NT)
	native_path = $(subst /,\,$(1))
else
	native_path = $(1)
endif

# Makedir command
ifeq ($(OS), Windows_NT)
	MKDIR_CMD = mkdir
else
	MKDIR_CMD = mkdir -p
endif
mkdir = -$(if $(wildcard $(1)/.*), , $(MKDIR_CMD) $(call native_path, $(1)) $(suppress_out))

# Rmdir command
ifeq ($(OS), Windows_NT)
	RMDIR_CMD = rmdir /s /q
else
	RMDIR_CMD = rm -rf
endif
rmdir = $(if $(wildcard $(1)/.*), $(RMDIR_CMD) $(call native_path, $(1)),)

# Lowercase
lc = $(subst A,a,$(subst B,b,$(subst C,c,$(subst D,d,$(subst E,e,$(subst F,f,$(subst G,g,\
	$(subst H,h,$(subst I,i,$(subst J,j,$(subst K,k,$(subst L,l,$(subst M,m,$(subst N,n,\
	$(subst O,o,$(subst P,p,$(subst Q,q,$(subst R,r,$(subst S,s,$(subst T,t,$(subst U,u,\
	$(subst V,v,$(subst W,w,$(subst X,x,$(subst Y,y,$(subst Z,z,$1))))))))))))))))))))))))))

# Quiet execution of command
quiet = $(if $(SILENT), $(suppress_out),)

# Newline macro
define \n


endef

# Os executable extension
ifeq ($(OS), Windows_NT)
	EXECEXT = .exe
else
	EXECEXT = .out
endif

#---------------------------------------------------------------
# Per project configuration
#---------------------------------------------------------------
# Should at least define:
# - PRJTYPE variable (Executable|StaticLib|DynLib)
# - LIBS variable (optional, Executable type only)
# Can optionally define:
# - TARGETNAME variable (project name, defaults to name of the root folder)
# - SRCDIR variable (source directory)
# - BUILDDIR variable (intermediate build directory)
# - SRC variable (list of the source files, defaults to every code file in SRCDIR)
# - SRCEXT variable (list of extensions used to match source files)
# - DEFINES variable (list defines in form of PROPERTY || PROPERTY=VALUE)
# - ADDINCS variable (list with additional include dirs)
# - MOREDEPS variable (list with additional dep dirs)
-include config.mk

# Defaults
TARGETNAME ?= $(notdir $(CURDIR))
SRCDIR ?= src
BUILDDIR ?= tmp
SRCEXT = *.c *.cpp *.cc *.cxx
SRC ?= $(call rwildcard, $(SRCDIR), $(SRCEXT))

# Target directory
ifeq ($(PRJTYPE), StaticLib)
	TARGETDIR = lib
else
	TARGETDIR = bin
endif

#---------------------------------------------------------------
# Toolchain dependent values
#---------------------------------------------------------------
ifeq ($(TOOLCHAIN), MSVC)
	# Compiler
	CC = cl
	CXX = cl
	CFLAGS = /nologo /EHsc /W4 /c
	CXXFLAGS =
	COUTFLAG = /Fo:
	# Preprocessor
	INCFLAG = /I
	DEFINEFLAG = /D
	# Archiver
	AR = lib
	ARFLAGS = /nologo
	SLIBEXT = .lib
	SLIBPREF =
	AROUTFLAG = /OUT:
	# Linker
	LD = link
	LDFLAGS = /nologo /manifest /entry:mainCRTStartup
	LIBFLAG =
	LIBSDIRFLAG = /LIBPATH:
	LOUTFLAG = /OUT:
	# Variant specific flags
	ifeq ($(VARIANT), Debug)
		CFLAGS += /MTd /Zi /Od /FS /Fd:$(BUILDDIR)/$(TARGETNAME).pdb
		LDFLAGS += /debug
	else
		CFLAGS += /MT /O2
		LDFLAGS += /incremental:NO
	endif
else
	# Compiler
	CC = gcc
	CXX = g++
	CFLAGS = -Wall -Wextra -c
	CXXFLAGS = -std=c++14
	COUTFLAG = -o
	# Preprocessor
	INCFLAG = -I
	DEFINEFLAG = -D
	# Archiver
	AR = ar
	ARFLAGS = rcs
	SLIBEXT = .a
	SLIBPREF = lib
	AROUTFLAG =
	# Linker
	LD = g++
	LDFLAGS =
	ifeq ($(OS), Windows_NT)
		LDFLAGS += -static -static-libgcc -static-libstdc++
	endif
	LIBFLAG = -l
	LIBSDIRFLAG = -L
	LOUTFLAG = -o
	# Variant specific flags
	ifeq ($(VARIANT), Debug)
		CFLAGS += -g -O0
	else
		CFLAGS += -O2
	endif
endif

# Preprocessor flags
CPPFLAGS = $(strip $(foreach define, $(DEFINES), $(DEFINEFLAG)$(define)))

#---------------------------------------------------------------
# Generated values
#---------------------------------------------------------------
# Objects
OBJEXT = .o
OBJ = $(foreach obj, $(SRC:=$(OBJEXT)), $(BUILDDIR)/$(VARIANT)/$(obj))
# Header dependencies
HDEPEXT = .d
HDEPS = $(OBJ:$(OBJEXT)=$(HDEPEXT))

# Output
ifeq ($(PRJTYPE), StaticLib)
	TARGET = $(SLIBPREF)$(strip $(call lc,$(TARGETNAME)))$(SLIBEXT)
else
	TARGET = $(TARGETNAME)$(EXECEXT)
endif

# Master output
ifdef PRJTYPE
	MASTEROUT = $(TARGETDIR)/$(VARIANT)/$(TARGET)
endif

# Dependencies
DEPSDIR = deps
depsgather = $(foreach d, $(wildcard $1/*), $d $(call depsgather, $d/$(strip $2), $2))
DEPS = $(strip $(sort $(call depsgather, $(DEPSDIR), $(DEPSDIR)))) $(MOREDEPS)
DEPNAMES = $(strip $(foreach d, $(DEPS), $(lastword $(subst /, , $d))))

# Include directories (implicit)
INCDIR = $(strip $(INCFLAG)include $(foreach dep, $(DEPS), $(INCFLAG)$(dep)/include))
# Include directories (explicit)
INCDIR += $(strip $(foreach addinc, $(ADDINCS), $(INCFLAG)$(addinc)))

# Library search directories
LIBSDIR = $(strip $(foreach libdir,\
			$(foreach dep, $(DEPS), $(dep)/lib) $(ADDLIBDIR),\
			$(LIBSDIRFLAG)$(libdir)/$(strip $(VARIANT))))
# Library flags
LIBFLAGS = $(strip $(foreach lib, $(LIBS), $(LIBFLAG)$(lib)$(if $(filter $(TOOLCHAIN), MSVC),.lib,)))

#---------------------------------------------------------------
# Command generator functions
#---------------------------------------------------------------
ccompile = $(CC) $$(CFLAGS) $$(CPPFLAGS) $$(INCDIR) $$< $(COUTFLAG) $$@
cxxcompile = $(CXX) $$(CFLAGS) $$(CXXFLAGS) $$(CPPFLAGS) $$(INCDIR) $$< $(COUTFLAG) $$@
link = $(LD) $(LDFLAGS) $(LIBSDIR) $(LOUTFLAG)$@ $^ $(LIBFLAGS)
archive = $(AR) $(ARFLAGS) $(AROUTFLAG)$@ $?

#---------------------------------------------------------------
# Colors
#---------------------------------------------------------------
ifneq ($(OS), Windows_NT)
	ESC = $(shell echo -e -n '\x1b')
endif
NO_COLOR=$(ESC)[0m
LGREEN_COLOR=$(ESC)[92m
LYELLOW_COLOR=$(ESC)[93m
LMAGENTA_COLOR=$(ESC)[95m
LRED_COLOR=$(ESC)[91m
DGREEN_COLOR=$(ESC)[32m
DYELLOW_COLOR=$(ESC)[33m
DCYAN_COLOR=$(ESC)[36m

#---------------------------------------------------------------
# Rules
#---------------------------------------------------------------
# Disable builtin rules
.SUFFIXES:

# Main build rule
build: variables $(OBJ) $(MASTEROUT)

# Full build rule
all: deps build

# Executes target
run: build
	$(eval exec = $(MASTEROUT))
	@echo Executing $(exec) ...
	@$(exec)

# Set variables for current build execution
variables:
	$(info $(LRED_COLOR)[o] Building$(NO_COLOR) $(LMAGENTA_COLOR)$(TARGETNAME)$(NO_COLOR))

# Print build debug info
showvars: variables
	@echo DEPS: $(DEPS)
	@echo DEPNAMES: $(DEPNAMES)
	@echo CFLAGS: $(CFLAGS)
	@echo LIBDIR: $(LIBDIR)
	@echo HDEPS: $(HDEPS)

# Link rule
%$(EXECEXT): $(OBJ)
	@$(info $(DGREEN_COLOR)[+] Linking$(NO_COLOR) $(DYELLOW_COLOR)$@$(NO_COLOR))
	@$(call mkdir, $(@D))
	$(eval lcommand = $(link))
	@$(lcommand)

# Archive rule
%$(SLIBEXT): $(OBJ)
	@$(info $(DCYAN_COLOR)[+] Archiving$(NO_COLOR) $(DYELLOW_COLOR)$@$(NO_COLOR))
	@$(call mkdir, $(@D))
	$(eval lcommand = $(archive))
	@$(lcommand)

# Command chunks that help generate dependency files in each toolchain
sed-escape = $(subst /,\/,$(subst \,\\,$(1)))
msvc-dep-gen = $(1) /showIncludes >$(basename $@)$(HDEPEXT) & \
	$(if $(SILENT),,sed -e "/^Note: including file:/d" $(basename $@)$(HDEPEXT) &&) \
	sed -i \
		-e "/: error /q1" \
		-e "/^Note: including file:/!d" \
		-e "s/^Note: including file:\s*\(.*\)$$/\1/" \
		-e "s/\\/\//g" \
		-e "s/ /\\ /g" \
		-e "s/^\(.*\)$$/\t\1 \\/" \
		-e "2 s/^.*$$/$(call sed-escape,$@)\: $(call sed-escape,$<) &/g" \
		$(basename $@)$(HDEPEXT) || (echo. >$(basename $@)$(HDEPEXT) & exit 1)
gcc-dep-gen = -MMD -MT $@ -MF $(basename $@)$(HDEPEXT)

# Command wrapper that adds dependency generation functionality to given compile command
ifndef NO_INC_BUILD
dep-gen-wrapper = $(if $(filter $(TOOLCHAIN), MSVC), \
	$(call msvc-dep-gen, $(1)), \
	$(1) $(gcc-dep-gen))
else
dep-gen-wrapper = $(1)
endif

#
# Compile rules
#
define compile-rule
$(BUILDDIR)/$(VARIANT)/%.$(strip $(1))$(OBJEXT): %.$(strip $(1))
	@$$(info $(LGREEN_COLOR)[>] Compiling$(NO_COLOR) $(LYELLOW_COLOR)$$<$(NO_COLOR))
	@$$(call mkdir, $$(@D))
	@$$(call dep-gen-wrapper, $(2)) $(quiet)
endef

# Generate compile rules
$(eval $(call compile-rule, c, $(ccompile)))
$(foreach ext, cpp cxx cc, $(eval $(call compile-rule, $(ext), $(cxxcompile))))

# Cleanup rule
clean:
	@echo Cleaning $(TARGETNAME)...
	@$(call rmdir, $(BUILDDIR))

# Dependencies build rule
deps:
	$(eval export SILENT=1)
	$(foreach dep, $(DEPS), @$(MAKE) -C $(dep) -f $(MKLOC) all${\n})

# Depencencies clean rule
depsclean:
	$(foreach dep, $(DEPS), @$(MAKE) -C $(dep) -f $(MKLOC) clean${\n})

# Don't create dependencies when we're cleaning
NOHDEPSGEN = clean $(foreach dep, $(DEPNAMES), clean-$(dep))
ifeq (0, $(words $(findstring $(MAKECMDGOALS), $(NOHDEPSGEN))))
	# GNU Make attempts to (re)build the file it includes
	-include $(HDEPS)
endif

# Non file targets
.PHONY: all \
		build \
		run \
		variables \
		showvars \
		clean \
		depsclean \
		deps
