PRJTYPE = StaticLib
SRC = \
	src/context.c \
	src/init.c    \
	src/input.c   \
	src/monitor.c \
	src/window.c
ifeq ($(OS), Windows_NT)
	SRC += \
		src/win32_init.c     \
		src/win32_monitor.c  \
		src/win32_time.c     \
		src/win32_tls.c      \
		src/win32_window.c   \
		src/winmm_joystick.c \
		src/wgl_context.c
else
	SRC += \
		src/x11_init.c       \
		src/x11_monitor.c    \
		src/x11_window.c     \
		src/xkb_unicode.c    \
		src/linux_joystick.c \
		src/posix_time.c     \
		src/posix_tls.c      \
		src/glx_context.c
endif

DEFINES = _GLFW_USE_OPENGL
ifeq ($(OS), Windows_NT)
	DEFINES += \
		_GLFW_WIN32 \
		_GLFW_WGL
else
	DEFINES += \
		_GLFW_X11 \
		_GLFW_GLX \
		_GLFW_HAS_GLXGETPROCADDRESS
endif

ADDINCS = contrib/GL/include
