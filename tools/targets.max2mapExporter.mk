MAX2MAP.SOURCE=\
	max2mapExporter/main.cc 

MAX2MAP.SOURCE.DEF=\
	max2mapExporter/exporter.def


MAX2MAP.SOURCE.OBJ:=$(addprefix $(DIR.OBJ)/, $(addsuffix $O, $(basename $(MAX2MAP.SOURCE))) )
SOURCE.ALL += $(MAX2MAP.SOURCE)


max2mapExporter: max2mapExporter$(EXT.DLE)

max2mapExporter$(EXT.DLE): CFLAGS += -I../engine -I../lib/maxsdk31

max2mapExporter$(EXT.DLE): LIB.PATH += \
	../lib/maxsdk31 \
	../lib/$(DIR.OBJ) \
	../engine/$(DIR.OBJ) \

max2mapExporter$(EXT.DLE): LINK.LIBS.GENERAL += \
	Mesh.lib        \
	Geom.lib        \
	MaxUtil.lib     \
	Gfx.lib         \
	Core.lib        \
	Bmm.lib         \
	ljpeg$(EXT.LIB) \
	lpng$(EXT.LIB) \
	lungif$(EXT.LIB) \
	zlib$(EXT.LIB) \
	engine$(EXT.LIB) \


max2mapExporter$(EXT.DLE): dirlist engine$(EXT.LIB) $(MAX2MAP.SOURCE.OBJ) $(MAX2MAP.SOURCE.DEF) 
	$(DO.LINK.DLL)




