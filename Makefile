RM=rm -f
CC=gcc
EXT=
CLIBS=

#OPT=-W -Wall -D_XOPEN_SOURCE=600 -D_XOPEN_SOURCE_EXTENTED -std=gnu99 -O3
OPT=-g -W -Wall -Wextra -std=gnu99 -O3 -fstack-protector-strong -D_FORTIFY_SOURCE=2 -D_REENTRANT -D_XOPEN_SOURCE=600 -D_XOPEN_SOURCE_EXTENTED -static-libgcc -static-libstdc++

VPATH=nilorea-library/src/
INCLUDE=nilorea-library/include

ALLEGRO_LIBS=-lallegro_acodec -lallegro_audio -lallegro_color -lallegro_image -lallegro_main -lallegro_primitives -lallegro_ttf -lallegro_font -lallegro
LIBNILOREA=-lnilorea64
CFLAGS+= -DALLEGRO_UNSTABLE

dir_name=$(shell date +%Y_%m_%d_%HH%MM%SS )

ifeq ($(OS),Windows_NT)
    CFLAGS+= -I$(INCLUDE) -D__USE_MINGW_ANSI_STDIO $(OPT)
	RM= del /Q
    CC= gcc
	ifeq (${MSYSTEM},MINGW32)
        RM=rm -f
        CFLAGS+= -m32
        EXT=.exe
        LIBNILOREA=-lnilorea32
        CLIBS=-IC:/msys64/mingw32/include -LC:/msys64/mingw32/lib
    endif
    ifeq (${MSYSTEM},MINGW64)
        RM=rm -f
        CFLAGS+= -DARCH64BITS
        EXT=.exe
        LIBNILOREA=-lnilorea64
        CLIBS=-IC:/msys64/mingw64/include -LC:/msys64/mingw64/lib
    endif
	ifeq (${MSYSTEM},MINGW64CB)
        RM=del /Q
        CFLAGS+= -DARCH64BITS
        EXT=.exe
        LIBNILOREA=-lnilorea64
        CLIBS=-IC:/msys64/mingw64/include -LC:/msys64/mingw64/lib
    endif
    CLIBS+= $(ALLEGRO_LIBS) -Wl,-Bstatic -lpthread  -Wl,-Bdynamic -lws2_32  -L../LIB/. #-mwindows
else
	LIBNILOREA=-lnilorea
	UNAME_S= $(shell uname -s)
	RM=rm -f
	CC=gcc
	EXT=
    ifeq ($(UNAME_S),Linux)
        CFLAGS+= -I$(INCLUDE) $(OPT)
        CLIBS+= $(ALLEGRO_LIBS) -lpthread -lm -no-pie
    endif
    ifeq ($(UNAME_S),SunOS)
        CC=cc
        CFLAGS+= -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -g -v -xc99 -I ../../LIB/include/ -mt -lm
        CLIBS+= $(ALLEGRO_LIBS) -lm -lsocket -lnsl -lpthread -lrt -L..
    endif
endif


SRC=n_common.c n_log.c n_str.c n_list.c n_time.c n_thread_pool.c n_3d.c n_particles.c cJSON.c states_management.c sledge_physics.c text_scroll.c GiftDash.c
OBJ=$(SRC:%.c=%.o)
.c.o:
	$(COMPILE.c) $<

%.o:%.c
	$(CC) $(CFLAGS) -o $@ -c $<

GiftDash$(EXT): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(CLIBS)

all: GiftDash$(EXT)

clean:
	$(RM) *.o
	$(RM) GiftDash$(EXT)
