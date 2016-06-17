PRJTYPE = Executable
LIBS = glfw glad macu
ifeq ($(OS), Windows_NT)
	LIBS += glu32 opengl32 gdi32 winmm ole32 user32 shell32
endif
