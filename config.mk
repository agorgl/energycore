PRJTYPE = Executable
LIBS = glfw glad macu
ifeq ($(TARGET_OS), Windows_NT)
	LIBS += glu32 opengl32 gdi32 winmm ole32 user32 shell32
else
	LIBS += GL GLU X11 Xrandr Xinerama Xcursor pthread dl
endif
EXTDEPS = macu::0.0.1dev
