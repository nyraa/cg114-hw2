CC = g++
CFLAGS = -Wall
LDFLAGS = -lGL -lGLU -lglut -lm -lGLEW

# Cross-compile (MinGW) settings for Windows .exe
CROSS_CC = x86_64-w64-mingw32-g++
CROSS_INCLUDES = -I/usr/local/x86_64-w64-mingw32/include
CROSS_LIBDIR = -L/usr/local/x86_64-w64-mingw32/lib
CROSS_CFLAGS = -DFREEGLUT_STATIC -DGLEW_STATIC
CROSS_LDFLAGS = -Wl,-Bstatic -lfreeglut_static -Wl,-Bdynamic -lopengl32 -lglu32 -lglew32 -lgdi32 -luser32 -lkernel32 -lwinmm -static-libgcc -static-libstdc++

OBJ = robotarm.o readstl.o math3d.o gltools.o
WIN_OBJ = robotarm_win.o readstl_win.o math3d_win.o gltools_win.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

%_win.o: %.c
	$(CROSS_CC) $(CFLAGS) $(CROSS_CFLAGS) $(CROSS_INCLUDES) -c $< -o $@

%_win.o: %.cpp
	$(CROSS_CC) $(CFLAGS) $(CROSS_CFLAGS) $(CROSS_INCLUDES) -c $< -o $@

all: robotarm

robotarm: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

# Windows target using MinGW cross-compiler
robotarm.exe: $(WIN_OBJ)
	$(CROSS_CC) $(CFLAGS) $(CROSS_CFLAGS) $(CROSS_INCLUDES) -o $@ $(WIN_OBJ) $(CROSS_LIBDIR) $(CROSS_LDFLAGS)

clean:
	rm -f robotarm robotarm.exe $(OBJ) $(WIN_OBJ)

.PHONY: all clean