#=- Makefile -=#
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
#  [5] contains executable results
#  [6] contains library results (static or shared)
#  [7] contains intermediate object files of compiling processes

# CreateProcess NULL bug
ifeq ($(OS), Windows_NT)
	SHELL = cmd.exe
endif

# Set current makefile location
MKLOC ?= $(CURDIR)/$(firstword $(MAKEFILE_LIST))
export MKLOC

#---------------------------------------------------------------
# Build variable parameters
#---------------------------------------------------------------
# Variant = (Debug|Release)
VARIANT ?= Debug
TOOLCHAIN ?= GCC
SILENT ?=
VERBOSE ?=
# Default target OS is host if not provided
TARGET_OS ?= $(OS)
# Install location
LOCAL_REPO ?= $(HOME)/.local

#---------------------------------------------------------------
# Helpers
#---------------------------------------------------------------
# Recursive wildcard func
rwildcard = $(foreach d, $(wildcard $1*), $(call rwildcard, $d/, $2) $(filter $(subst *, %, $2), $d))

# Suppress full command output
ifeq ($(OS), Windows_NT)
	suppress_out = > nul 2>&1
else
	suppress_out = > /dev/null 2>&1
endif

# Ignore command error
ifeq ($(OS), Windows_NT)
	ignore_err = || (exit 0)
else
	ignore_err = || true
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
mkdir = -$(if $(wildcard $(1)/.*), , $(MKDIR_CMD) $(call native_path, $(1)) $(suppress_out)$(ignore_err))

# Rmdir command
ifeq ($(OS), Windows_NT)
	RMDIR_CMD = rmdir /s /q
else
	RMDIR_CMD = rm -rf
endif
rmdir = $(if $(wildcard $(1)/.*), $(RMDIR_CMD) $(call native_path, $(1)),)

# Copy command
ifeq ($(OS), Windows_NT)
	copy = robocopy $(call native_path, $(1)) $(call native_path, $(2)) /s /e
else
	copy = cp -r $(call native_path, $(1)) $(call native_path, $(2))
endif

# Path separator
pathsep = $(strip $(if $(filter $(OS), Windows_NT), ;, :))

# Lowercase
lc = $(subst A,a,$(subst B,b,$(subst C,c,$(subst D,d,$(subst E,e,$(subst F,f,$(subst G,g,\
	$(subst H,h,$(subst I,i,$(subst J,j,$(subst K,k,$(subst L,l,$(subst M,m,$(subst N,n,\
	$(subst O,o,$(subst P,p,$(subst Q,q,$(subst R,r,$(subst S,s,$(subst T,t,$(subst U,u,\
	$(subst V,v,$(subst W,w,$(subst X,x,$(subst Y,y,$(subst Z,z,$1))))))))))))))))))))))))))

# Quiet execution of command
quiet = $(if $(SILENT), $(suppress_out),)

# Command shown when executed
showcmd = $(if $(VERBOSE),,@)

# Newline macro
define \n


endef

# Space variable
space := $(subst ,, )

# Comma variable
comma := ,

# Canonical path (1 = base, 2 = path)
canonical_path = $(filter-out $(1), $(patsubst $(strip $(1))/%,%,$(abspath $(2))))
canonical_path_cur = $(call canonical_path, $(CURDIR), $(1))

# Gather given file from all subdirectories (1 = base, 2 = file)
findfile = $(foreach d, $(wildcard $1/*), $(filter %$(strip $2), $d))
# Gather given file from all subdirectories recursively (1 = base, 2 = file)
rfindfile = $(foreach d, $(wildcard $1/*), $(filter %$(strip $2), $d) $(call rfindfile, $d, $2))
# Gather all project configs
gatherprojs = $(strip $(patsubst ./%, %, $(patsubst %/config.mk, %, $(sort $(call rfindfile, $1, config.mk)))))

# Search for library in given paths
searchlibrary = $(foreach sd, $2, $(call findfile, $(sd), $1))
# Parse libraryname::version pairs
lib-from-extlib-pair = $(firstword $(subst ::, , $(1)))
ver-from-extlib-pair = $(strip $(or $(lastword $(subst ::, , $(1))), dev))
# Construct base path to repo dependency
extdep-pair = $(strip $(call lc,$(1)))::$(strip $(2))
extdep-path = $(LOCAL_REPO)/builds/$(subst ::,/,$(call extdep-pair, $(1), $(2)))
extdep-conf = $(call extdep-path, \
					$(call lib-from-extlib-pair, $(1)), \
					$(call ver-from-extlib-pair, $(1)))/pkg.mk

#---------------------------------------------------------------
# Global constants
#---------------------------------------------------------------
# Os executable extension
ifeq ($(TARGET_OS), Windows_NT)
	EXECEXT = .exe
else
	EXECEXT = .out
endif
# Os dynamic library extension
ifeq ($(TARGET_OS), Windows_NT)
	DLEXT = .dll
else
	DLEXT = .so
endif
# Object file extension
OBJEXT = .o
# Headers dependency file extension
HDEPEXT = .d
# Intermediate build directory
BUILDDIR := tmp

#---------------------------------------------------------------
# Header dependency generation helpers
#---------------------------------------------------------------
# Command chunks that help generate dependency files in each toolchain
sed-escape = $(subst /,\/,$(subst \,\\,$(1)))
msvc-dep-gen = $(1) /showIncludes >$(basename $@)$(HDEPEXT) & \
	$(if $(strip $(2)),,sed -e "/^Note: including file:/d" $(basename $@)$(HDEPEXT) &&) \
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
	$(call msvc-dep-gen, $(1), $(2)), \
	$(1) $(gcc-dep-gen))
else
dep-gen-wrapper = $(1)
endif

#---------------------------------------------------------------
# Colors
#---------------------------------------------------------------
ifneq ($(OS), Windows_NT)
	ESC := $(shell echo -e -n '\x1b')
endif
NO_COLOR       := $(ESC)[0m
LGREEN_COLOR   := $(ESC)[92m
LYELLOW_COLOR  := $(ESC)[93m
LMAGENTA_COLOR := $(ESC)[95m
LRED_COLOR     := $(ESC)[91m
DGREEN_COLOR   := $(ESC)[32m
DYELLOW_COLOR  := $(ESC)[33m
DCYAN_COLOR    := $(ESC)[36m

#---------------------------------------------------------------
# Toolchain dependent values
#---------------------------------------------------------------
ifeq ($(TOOLCHAIN), MSVC)
	# Compiler
	CC          := cl
	CXX         := cl
	CFLAGS      := /nologo /EHsc /W4 /c
	CXXFLAGS    :=
	COUTFLAG    := /Fo:
	# Preprocessor
	INCFLAG     := /I
	DEFINEFLAG  := /D
	# Archiver
	AR          := lib
	ARFLAGS     := /nologo
	SLIBEXT     := .lib
	SLIBPREF    :=
	AROUTFLAG   := /OUT:
	# Linker
	LD          := link
	LDFLAGS     := /nologo /manifest
	LIBFLAG     :=
	LIBSDIRFLAG := /LIBPATH:
	LOUTFLAG    := /OUT:
	LSOFLAGS    := /DLL
	# Variant specific flags
	ifeq ($(VARIANT), Debug)
		CFLAGS  += /MTd /Zi /Od /FS /Fd:$(BUILDDIR)/$(TARGETNAME).pdb
		LDFLAGS += /debug
	else
		CFLAGS  += /MT /O2
		LDFLAGS += /incremental:NO
	endif
else
	# Compiler
	CC          := gcc
	CXX         := g++
	CFLAGS      := -Wall -Wextra -c
	CXXFLAGS    := -std=c++14
	COUTFLAG    := -o
	CSOFLAGS    := -fPIC
	# Preprocessor
	INCFLAG     := -I
	DEFINEFLAG  := -D
	# Archiver
	AR          := ar
	ARFLAGS     := rcs
	SLIBEXT     := .a
	SLIBPREF    := lib
	AROUTFLAG   :=
	# Linker
	LD          := g++
	LDFLAGS     :=
	ifeq ($(TARGET_OS), Windows_NT)
		LDFLAGS += -static-libgcc -static-libstdc++
	endif
	LIBFLAG     := -l
	LIBSDIRFLAG := -L
	LOUTFLAG    := -o
	LSOFLAGS    := -shared -fPIC
	# Variant specific flags
	ifeq ($(VARIANT), Debug)
		CFLAGS  += -g -O0
	else
		CFLAGS  += -O2
	endif
endif

ifdef CROSS_COMPILE
	CC  := $(CROSS_COMPILE)-$(CC)
	CXX := $(CROSS_COMPILE)-$(CXX)
	AR  := $(CROSS_COMPILE)-$(AR)
	LD  := $(CROSS_COMPILE)-$(LD)
	CFLAGS   += --sysroot=$(SYSROOT)
	CXXFLAGS += --sysroot=$(SYSROOT)
	CPPFLAGS += --sysroot=$(SYSROOT)
	LDFLAGS  += --sysroot=$(SYSROOT)
endif

#---------------------------------------------------------------
# Rule generators
#---------------------------------------------------------------
# Compile rules (1 = extension, 2 = command generator, 3 = subproject path)
define compile-rule
$(BUILDDIR)/$(VARIANT)/$(strip $(3))%.$(strip $(1))$(OBJEXT): $(strip $(3))%.$(strip $(1))
	@$$(info $(LGREEN_COLOR)[>] Compiling$(NO_COLOR) $(LYELLOW_COLOR)$$<$(NO_COLOR))
	@$$(call mkdir, $$(@D))
	$(showcmd)$$(call dep-gen-wrapper, $(2), $(SILENT)) $(quiet)
endef

#---------------------------------------------------------------
# External dependency resolution
#---------------------------------------------------------------
define read-ext
-include $$(call extdep-conf, $(1))
PKGS += $$(PKGDEPS)
endef

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

define gen-build-rules
# Subproject name
$(eval D = $(strip $(1)))
# Subproject path
$(eval DP = $(if $(filter $(D),.),,$(D)/))

# Clear previous variables
$(foreach v, PRJTYPE VERSION LIBS SRCDIR SRC DEFINES ADDINCS MOREDEPS EXTDEPS, undefine $(v)${\n})

# Include configuration
-include $(DP)config.mk

# Gather variables from config
PRJTYPE_$(D)   := $$(PRJTYPE)
VERSION_$(D)   := $$(VERSION)
SRCDIR_$(D)    := $$(foreach d, $$(SRCDIR), $(DP)$$(d))
SRC_$(D)       := $$(foreach s, $$(SRC), $(DP)$$(s))
DEFINES_$(D)   := $$(DEFINES)
ADDINCS_$(D)   := $$(foreach ai, $$(ADDINCS), $(DP)$$(ai))
ADDLIBDIR_$(D) := $$(foreach ald, $$(ADDLIBDIR), $(DP)$$(ald))
LIBS_$(D)      := $$(foreach l, $$(LIBS), $$(l))
MOREDEPS_$(D)  := $$(MOREDEPS)
EXTDEPS_$(D)   := $$(EXTDEPS)

# Set defaults on unset variables
VERSION_$(D)    := $$(strip $$(or $$(VERSION_$(D)), dev))
TARGETNAME_$(D) := $$(or $$(TARGETNAME_$(D)), $$(notdir $$(if $$(filter $(D),.), $$(CURDIR), $(D))))
SRCDIR_$(D)     := $$(or $$(SRCDIR_$(D)), $(DP)src)
SRCEXT_$(D)     := *.c *.cpp *.cc *.cxx
SRC_$(D)        := $$(or $$(SRC_$(D)), $$(call rwildcard, $$(SRCDIR_$(D)), $$(SRCEXT_$(D))))

# Target directory
ifeq ($$(PRJTYPE_$(D)), Executable)
	TARGETDIR_$(D) := $(DP)bin
else
	TARGETDIR_$(D) := $(DP)lib
endif

#---------------------------------------------------------------
# Generated values
#---------------------------------------------------------------
# Objects
OBJ_$(D) := $$(foreach obj, $$(SRC_$(D):=$(OBJEXT)), $(BUILDDIR)/$(VARIANT)/$$(obj))
# Header dependencies
HDEPS_$(D) := $$(OBJ_$(D):$(OBJEXT)=$(HDEPEXT))

# Output
ifeq ($$(PRJTYPE_$(D)), StaticLib)
	TARGET_$(D) := $(SLIBPREF)$$(strip $$(call lc,$$(TARGETNAME_$(D))))$(SLIBEXT)
else ifeq ($$(PRJTYPE_$(D)), DynLib)
	TARGET_$(D) := $(SLIBPREF)$$(strip $$(call lc,$$(TARGETNAME_$(D))))$(DLEXT)
else
	TARGET_$(D) := $$(TARGETNAME_$(D))$(EXECEXT)
endif

# Master output
ifdef PRJTYPE_$(D)
	MASTEROUT_$(D) := $$(TARGETDIR_$(D))/$(VARIANT)/$$(TARGET_$(D))
endif

# Install location
INSTALL_PREFIX_$(D) := $$(call extdep-path, $$(TARGETNAME_$(D)), $$(VERSION_$(D)))
# Install pair
INSTALL_PAIR_$(D) := $$(call extdep-pair, $$(TARGETNAME_$(D)), $$(VERSION_$(D)))

# Implicit dependencies directory
DEPSDIR_$(D) := $(DP)deps
# Implicit dependencies
DEPS_$(D) := $$(call gatherprojs, $$(DEPSDIR_$(D)))
# Explicit dependencies
DEPS_$(D) += $$(foreach md, $$(MOREDEPS_$(D)), $$(or $$(call canonical_path_cur, $(DP)/$$(md)), .))

# Preprocessor flags
CPPFLAGS_$(D) := $$(strip $$(foreach define, $$(DEFINES_$(D)), $(DEFINEFLAG)$$(define)))

# Append dependency pairs from local dependencies
undefine PKGS
PKGS :=
$$(foreach dep, $$(EXTDEPS_$(D)), $$(eval $$(call read-ext, $$(dep))))
EXTDEPS_$(D) += $$(PKGS)

# External (repo installed) dependency directories
EXTDEPPATHS_$(D) := $$(foreach ed, $$(EXTDEPS_$(D)), $$(call extdep-path, \
						$$(call lib-from-extlib-pair, $$(ed)), \
						$$(call ver-from-extlib-pair, $$(ed))))

# Include search directories
INCPATHS_$(D) := $$(strip $(DP)include \
						$$(foreach dep, $$(DEPS_$(D)) \
										$$(filter-out $$(DEPS_$(D)), $$(wildcard $$(DEPSDIR_$(D))/*)), \
											$$(dep)/include) \
						$$(ADDINCS_$(D)) \
						$$(foreach extdep, $$(EXTDEPPATHS_$(D)), $$(extdep)/include))
# Include path flags
INCDIR_$(D) := $$(strip $$(foreach inc, $$(INCPATHS_$(D)), $(INCFLAG)$$(inc)))

# Library search paths
LIBPATHS_$(D) := $$(strip $$(foreach libdir,\
									$$(foreach dep, $$(DEPS_$(D)), $$(dep)/lib) \
									$$(ADDLIBDIR_$(D)),\
								$$(libdir)/$(strip $(VARIANT))) \
								$$(foreach extdep, $$(EXTDEPPATHS_$(D)), $$(extdep)/lib))
# Library path flags
LIBSDIR_$(D) := $$(strip $$(foreach lp, $$(LIBPATHS_$(D)), $(LIBSDIRFLAG)$$(lp)))

# Library flags
LIBFLAGS_$(D) := $$(strip $$(foreach lib, $$(LIBS_$(D)), $(LIBFLAG)$$(lib)$(if $(filter $(TOOLCHAIN), MSVC),.lib,)))

# Extra link flags when building shared libraries
ifeq ($$(PRJTYPE_$(D)), DynLib)
	# Add shared library toggle
	MORELFLAGS_$(D) := $(LSOFLAGS)
endif

# Setup rpath flag parameter for linux systems
ifeq ($$(PRJTYPE_$(D)), Executable)
ifneq ($(TARGET_OS), Windows_NT)
	# Path from executable location to project root
	RELPPREFIX_$(D) := $$(subst $$(space),,$$(foreach dir, $$(subst /,$$(space),$$(dir $$(MASTEROUT_$(D)))),../))
	# Library paths relative to the executable
	LIBRELPATHS_$(D) := $$(strip $$(foreach p, $$(LIBPATHS_$(D)), $$(RELPPREFIX_$(D))$$(p)))
	# Add rpath param to search for dependent shared libraries relative to the executable location
	MORELFLAGS_$(D) := '-Wl$$(comma)-rpath$$(comma)$$(subst $$(space),:,$$(addprefix $$$$ORIGIN/, $$(LIBRELPATHS_$(D))))'
endif
endif

# Build rule dependencies
BUILDDEPS_$(D) := $$(foreach dep, $$(DEPS_$(D)), build_$$(strip $$(dep)))

# Link rule dependencies
ifneq ($$(PRJTYPE_$(D)), StaticLib)
LIBDEPS_$(D) = $$(foreach dep, $$(DEPS_$(D)), \
					$$(if $$(or $$(filter $$(PRJTYPE_$$(strip $$(dep))), StaticLib), \
								$$(filter $$(PRJTYPE_$$(strip $$(dep))), DynLib)), \
						$$(MASTEROUT_$$(strip $$(dep)))))
endif

# Install dependencies for static library
ifeq ($$(PRJTYPE_$(D)), StaticLib)
INSTDEPS_$(D) := $$(foreach dep, $$(DEPS_$(D)), install_$$(strip $$(dep)))
endif

#---------------------------------------------------------------
# Rules
#---------------------------------------------------------------
# Main build rule
build_$(D): $$(MASTEROUT_$(D))

# Executes target
run_$(D): build_$(D)
	@echo Executing $$(MASTEROUT_$(D)) ...
	@$$(eval export PATH := $(PATH)$(pathsep)$$(subst $$(space),$(pathsep),$$(addprefix $$(CURDIR)/, $$(LIBPATHS_$(D)))))
	@cd $(D) && $$(call native_path, $$(call canonical_path, $$(abspath $$(CURDIR)/$(D)), $$(MASTEROUT_$(D))))

ifneq ($$(PRJTYPE_$(D)), Executable)
# Pkg.mk file contents
define PCFG_$(D)
PKGDEPS := $$(strip $$(EXTDEPS_$(D)) $$(foreach dep, $$(DEPS_$(D)), $$(INSTALL_PAIR_$$(dep))))
endef

# Installs target to repository
install_$(D): $$(INSTDEPS_$(D)) $$(MASTEROUT_$(D))
	$$(info $(LGREEN_COLOR)[>] Installing$(NO_COLOR) $(LYELLOW_COLOR)$$(TARGETNAME_$(D)) to $$(INSTALL_PREFIX_$(D))$(NO_COLOR))
	$(showcmd)$$(call mkdir, $$(INSTALL_PREFIX_$(D))/lib)
	$(showcmd)$$(call mkdir, $$(INSTALL_PREFIX_$(D))/include)
	$(showcmd)$$(call copy, $(DP)include/*, $$(INSTALL_PREFIX_$(D))/include)
	$(showcmd)$$(call copy, $$(MASTEROUT_$(D)), $$(INSTALL_PREFIX_$(D))/lib)
	$(showcmd)echo "$$(PCFG_$(D))" > $$(call extdep-conf, $$(INSTALL_PAIR_$(D)))
endif

# Show banner for current build execution
banner_$(D):
	$$(info $(LRED_COLOR)[o] Building$(NO_COLOR) $(LMAGENTA_COLOR)$$(TARGETNAME_$(D))$(NO_COLOR))

# Build objects for target
objects_$(D): $$(OBJ_$(D))

# Print build debug info
showvars_$(D): banner_$(D)
	@echo MASTEROUT: $$(MASTEROUT_$(D))
	@echo SRCDIR:    $$(SRCDIR_$(D))
	@echo SRC:       $$(SRC_$(D))
	@echo DEPS:      $$(DEPS_$(D))
	@echo BUILDDEPS: $$(BUILDDEPS_$(D))
	@echo CFLAGS:    $$(CFLAGS)
	@echo CPPFLAGS:  $$(CPPFLAGS_$(D))
	@echo INCDIR:    $$(INCDIR_$(D))
	@echo LIBSDIR:   $$(LIBSDIR_$(D))
	@echo LIBFLAGS:  $$(LIBFLAGS_$(D))
	@echo LIBDEPS:   $$(LIBDEPS_$(D))
	@echo HDEPS:     $$(HDEPS_$(D))
	@echo INSTALL:   $$(INSTALL_PREFIX_$(D))
	@echo EXTDEPS:   $$(EXTDEPS_$(D))
	@echo EXTPATHS:  $$(EXTDEPPATHS_$(D))

$$(MASTEROUT_$(D)): $$(BUILDDEPS_$(D)) banner_$(D) objects_$(D)
ifneq ($$(PRJTYPE_$(D)), StaticLib)
# Link rule
	@$$(info $(DGREEN_COLOR)[+] Linking$(NO_COLOR) $(DYELLOW_COLOR)$$@$(NO_COLOR))
	@$$(call mkdir, $$(@D))
	$(showcmd)$(LD) $(LDFLAGS) $$(LIBSDIR_$(D)) $(LOUTFLAG)$$@ $$(OBJ_$(D)) $$(LIBFLAGS_$(D)) $$(MORELFLAGS_$(D))
else
# Archive rule
	@$$(info $(DCYAN_COLOR)[+] Archiving$(NO_COLOR) $(DYELLOW_COLOR)$$@$(NO_COLOR))
	@$$(call mkdir, $$(@D))
	$(showcmd)$(AR) $(ARFLAGS) $(AROUTFLAG)$$@ $$(OBJ_$(D))
endif

# Compile commands
CCOMPILE_$(D)   = $(CC) $(CFLAGS) $$(CPPFLAGS_$(D)) $$(INCDIR_$(D)) $$< $(COUTFLAG) $$@ $$(MORECFLAGS_$(D))
CXXCOMPILE_$(D) = $(CXX) $(CFLAGS) $(CXXFLAGS) $$(CPPFLAGS_$(D)) $$(INCDIR_$(D)) $$< $(COUTFLAG) $$@ $$(MORECFLAGS_$(D))

# Generate compile rules
$(call compile-rule, c, $$(CCOMPILE_$(D)), $(DP))
$(foreach ext, cpp cxx cc, $(call compile-rule, $(ext), $$(CXXCOMPILE_$(D)), $(DP))${\n})

# Include extra rules
-include $(DP)rules.mk

endef

#---------------------------------------------------------------
# Generation
#---------------------------------------------------------------
# Scan all directories for subprojects
SUBPROJS := $(call gatherprojs, .)
# Create sublists with dependency and main projects
SILENT_SUBPROJS = $(foreach subproj, $(SUBPROJS), $(if $(findstring deps, $(subproj)), $(subproj)))
NONSILENT_SUBPROJS = $(filter-out $(SILENT_SUBPROJS), $(SUBPROJS))
# Generate subproject variables and rules
SILENT := 1
$(foreach subproj, $(SILENT_SUBPROJS), $(eval $(call gen-build-rules, $(subproj))))
undefine SILENT
$(foreach subproj, $(NONSILENT_SUBPROJS), $(eval $(call gen-build-rules, $(subproj))))

# Aliases
build: build_.
run: run_.
install: install_.
showvars: showvars_.
showvars_all: $(foreach subproj, $(SUBPROJS), showvars_$(subproj))

# Track down Dynamic Library projects and find their dependencies
SO_PROJS := $(strip $(foreach subproj, $(SUBPROJS), $(if $(filter $(PRJTYPE_$(subproj)), DynLib), $(subproj),)))
SO_DEPS  := $(strip $(sort $(foreach sop, $(SO_PROJS), $(foreach dep, $(DEPS_$(sop)), $(if $(filter $(PRJTYPE_$(dep)), StaticLib), $(dep),)))))
# Make these projects have position independent code
$(foreach pic, $(SO_PROJS) $(SO_DEPS), $(eval MORECFLAGS_$(pic) += $(CSOFLAGS)))

# Cleanup rule
clean:
	@echo Cleaning...
	@$(call rmdir, $(BUILDDIR))

# Don't create dependencies when we're cleaning
NOHDEPSGEN = clean
ifeq (0, $(words $(findstring $(MAKECMDGOALS), $(NOHDEPSGEN))))
	# GNU Make attempts to (re)build the file it includes
	HDEPS = $(foreach subproj, $(SUBPROJS), $(HDEPS_$(subproj)))
	-include $(HDEPS)
endif

# Set default goal
.DEFAULT_GOAL := build

# Disable builtin rules
.SUFFIXES:

# Non file targets
PHONYRULETYPES := run showvars
PHONYPREREQS := $(foreach ruletype, $(PHONYRULETYPES), $(foreach subproj, $(SUBPROJS), $(ruletype)_$(subproj))) \
		run \
		showvars \
		showvars_all \
		clean
.PHONY: $(PHONYPREREQS)
.SECONDARY:
