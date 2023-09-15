BUILDWAD.SOURCE=\
	buildWad/main.cc \
	buildWad/wadProcessor.cc \
	buildWad/palQuantization.cc 


BUILDWAD.SOURCE.OBJ:=$(addprefix $(DIR.OBJ)/, $(addsuffix $O, $(basename $(BUILDWAD.SOURCE))) )
SOURCE.ALL += $(BUILDWAD.SOURCE)


buildWad: buildWad$(EXT.EXE)

buildWad$(EXT.EXE): CFLAGS += -I../engine -I../lib/zlib -I../lib/lungif -I../lib/lpng -I../lib/ljpeg 

buildWad$(EXT.EXE): LIB.PATH += \
	../engine/$(DIR.OBJ) \
	../lib/$(DIR.OBJ) \

buildWad$(EXT.EXE): LINK.LIBS.GENERAL = $(PRE.ENGINE.LIB)engine$(EXT.LIB) \
	$(PRE.LIBRARY.LIB)ljpeg$(EXT.LIB) \
	$(PRE.LIBRARY.LIB)lpng$(EXT.LIB) \
	$(PRE.LIBRARY.LIB)lungif$(EXT.LIB) \
	$(PRE.LIBRARY.LIB)zlib$(EXT.LIB) \
	$(LINK.LIBS.TOOLS)


buildWad$(EXT.EXE): dirlist engine$(EXT.LIB) $(BUILDWAD.SOURCE.OBJ)
	$(DO.LINK.CONSOLE.EXE)
