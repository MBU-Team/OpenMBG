TEX2BM8.SOURCE=\
	texture2bm8/main.cc \
	texture2bm8/svector.cc \
	texture2bm8/vector.asm


TEX2BM8.SOURCE.OBJ:=$(addprefix $(DIR.OBJ)/, $(addsuffix $O, $(basename $(TEX2BM8.SOURCE))) )
SOURCE.ALL += $(TEX2BM8.SOURCE)

texture2bm8: texture2bm8$(EXT.EXE)

texture2bm8$(EXT.EXE): CFLAGS += -I../engine -I../lib/zlib -I../lib/lungif -I../lib/lpng -I../lib/ljpeg

texture2bm8$(EXT.EXE): LIB.PATH += \
	../engine/$(DIR.OBJ) \
	../lib/$(DIR.OBJ) \

texture2bm8$(EXT.EXE): LINK.LIBS.GENERAL = $(PRE.ENGINE.LIB)engine$(EXT.LIB) \
	$(PRE.LIBRARY.LIB)ljpeg$(EXT.LIB) \
	$(PRE.LIBRARY.LIB)lpng$(EXT.LIB) \
	$(PRE.LIBRARY.LIB)lungif$(EXT.LIB) \
	$(PRE.LIBRARY.LIB)zlib$(EXT.LIB) \
	$(LINK.LIBS.TOOLS)

texture2bm8$(EXT.EXE): dirlist engine$(EXT.LIB) $(TEX2BM8.SOURCE.OBJ)
	$(DO.LINK.CONSOLE.EXE)




