PRJTYPE = Executable
LIBS = glfw glad
ifeq ($(OS), Windows_NT)
	LIBS += glu32 opengl32 gdi32 winmm ole32
endif
