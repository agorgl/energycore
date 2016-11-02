PRJTYPE = StaticLib
SRC = \
	src/context.c \
	src/init.c    \
	src/input.c   \
	src/monitor.c \
	src/window.c  \
	src/vulkan.c
ifeq ($(TARGET_OS), Windows_NT)
	SRC += \
		src/win32_init.c     \
		src/win32_monitor.c  \
		src/win32_time.c     \
		src/win32_tls.c      \
		src/win32_window.c   \
		src/win32_joystick.c \
		src/wgl_context.c    \
		src/egl_context.c
else
	SRC += \
		src/x11_init.c       \
		src/x11_monitor.c    \
		src/x11_window.c     \
		src/xkb_unicode.c    \
		src/linux_joystick.c \
		src/posix_time.c     \
		src/posix_tls.c      \
		src/glx_context.c    \
		src/egl_context.c
endif

DEFINES =
ifeq ($(TARGET_OS), Windows_NT)
	DEFINES += _GLFW_WIN32
else
	DEFINES += _GLFW_X11
endif
