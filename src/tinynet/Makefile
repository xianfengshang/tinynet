
#
# c、cpp混合编译的makefile模板
#


PLAT = none

TARGET_NAME = tinynet
PDB_NAME = $(TARGET_NAME).debuginfo
CC = $(CROSS)gcc
CXX = $(CROSS)g++
#CC = $(CROSS)clang
#CXX = $(CROSS)clang++
AR = $(CROSS)ar rcu
RANLIB = $(CROSS)ranlib
STRIP = $(CROSS)strip
RM = rm -f
CP = cp -r
OBJCOPY = $(CROSS)objcopy
CP_STRIP_DEBUG = $(OBJCOPY) --strip-debug
CP_ONLY_DEBUG = $(OBJCOPY) --only-keep-debug

CFLAGS = $(CROSS_FLAGS) $(EXTRA_CFLAGS)
CXXFLAGS = $(CROSS_FLAGS) $(EXTRA_CXXFLAGS)

EXTRA_CFLAGS = 
EXTRA_CXXFLAGS = 

EXTRA_INCS = 
EXTRA_LIBS = 


SRC_INCS = -I"../../3rd/" -I"../../3rd/curl/" -I"../../3rd/jsoncpp/include/" -I"../../3rd/linux" -I"../../3rd/protobuf-2.6.1/include/" -I"../../base" -I"../../util" -I"./" -I"../" -I"../../3rd/pugixml" -I"../../3rd/libyaml/include" -I"../../3rd/libzip" -I"../../3rd/rapidjson/include" -I"../../3rd/linux/jemalloc/include" -I"../../3rd/boost-1_72" -I"../../3rd/libmaxminddb/include"

SRC_LIBS = -L"../../3rd/lib64" -ljemalloc -lcurl -lprotobuf -lpthread -lrt -ldl -lssl -lcrypto -lz -lcjson -lluasocket -lpugixml -lyaml -lzip -lmaxminddb

C_SRC_ALL = $(wildcard ./*.c ../net/http/*.c)

CXX_SRC_ALL = $(wildcard ./*.cpp ../base/*.cpp ../util/*.cpp ../lualib/*.cpp ../net/*.cpp ../net/http/*.cpp ../net/websocket/*.cpp ../redis/*.cpp ../rpc/*.cpp ../app/*.cpp ../tns/*.cpp ../tdc/*.cpp ../mysql/*.cpp ../raft/*.cpp ../ipc/*.cpp ../ipc/rpc/*.cpp ../logging/*.cpp ../cluster/*.cpp ../tfs/*.cpp ../text/*.cpp ../process/*.cpp ../io/*.cpp ../wal/*.cpp ../naming/*.cpp ../geo/*.cpp ../aoi/*.cpp ../tilemap/*.cpp)

CC_SRC_ALL = $(wildcard ../base/*.cc ../tdc/*.cc ../naming/*.cc ../raft/*.cc ../logging/*.cc)

TARGET_DIR = ../../bin/
PLATS = linux-debug linux-release
none:
	@echo "Please choose a platform:"
	@echo " $(PLATS)"

linux-debug:
	$(MAKE) all EXTRA_INCS='-I"../../3rd/lua-5.4.6/include/"' EXTRA_LIBS='-L"../../3rd/lib64/lua54" -llua' CROSS_FLAGS="-Wall -DDEBUG -DUSE_JEMALLOC -DLUA_COMPAT_APIINTCASTS -g" EXTRA_CXXFLAGS="-std=c++11"
	
linux-release:
	$(MAKE) all EXTRA_INCS='-I"../../3rd/lua-5.4.6/include/"' EXTRA_LIBS="-L"../../3rd/lib64/lua54" -llua" CROSS_FLAGS="-Wall -DNDEBUG -DUSE_JEMALLOC -DLUA_COMPAT_APIINTCASTS -O2 -g" EXTRA_CXXFLAGS="-std=c++11"

linux-debugjit:
	$(MAKE) all EXTRA_INCS='-I"../../3rd/LuaJIT-2.1.0-beta3/include/"' EXTRA_LIBS='-L"../../3rd/lib64/luaJIT" -lluajit' CROSS_FLAGS="-Wall -DDEBUG -DUSE_JEMALLOC -g" EXTRA_CXXFLAGS="-std=c++11"
	
linux-releasejit:
	$(MAKE) all EXTRA_INCS='-I"../../3rd/LuaJIT-2.1.0-beta3/include/"' EXTRA_LIBS='-L"../../3rd/lib64/luaJIT" -lluajit' CROSS_FLAGS="-Wall -DNDEBUG -DUSE_JEMALLOC -O2 -g" EXTRA_CXXFLAGS="-std=c++11"

linux-valgrind:
	$(MAKE) all EXTRA_INCS='-I"../../3rd/LuaJIT-2.1.0-beta3/include/"' EXTRA_LIBS='-L"../../3rd/lib64/luaJIT" -lluajit_valgrind' CROSS_FLAGS="-Wall -DNDEBUG -DUSE_JEMALLOC -O0 -g" EXTRA_CXXFLAGS="-std=c++11"

linux-profile:
	$(MAKE) all EXTRA_INCS='-I"../../3rd/LuaJIT-2.1.0-beta3/include/"' EXTRA_LIBS='-L"../../3rd/lib64/luaJIT" -lluajit' CROSS_FLAGS="-Wall -DNDEBUG  -DUSE_JEMALLOC -O0 -pg -g" EXTRA_CXXFLAGS="-std=c++11"
	
all:$(TARGET_NAME)


echo:
	@echo "PLAT = $(PLAT)"
	@echo "CC = $(CC)"
	@echo "CXX = $(CXX)"
	@echo "AR = $(AR)"
	@echo "RANLIB = $(RANLIB)"
	@echo "RM = $(RM)"
	@echo "CP = $(CP)"
	@echo "CFLAGS = $(CFLAGS)"
	@echo "CXXFLAGS = $(CXXFLAGS)"
	@echo "EXTRA_CFLAGS = $(EXTRA_CFLAGS)"
	@echo "EXTRA_CXXFLAGS = $(EXTRA_CXXFLAGS)"



C_OBJ_ALL := $(C_SRC_ALL:.c=.o)
CXX_OBJ_ALL := $(CXX_SRC_ALL:.cpp=.o)
CC_OBJ_ALL := $(CC_SRC_ALL:.cc=.o)

#$(OBJS):%.o :%.c  先用$(OBJS)中的一项，比如foo.o: %.o : %.c  含义为:试着用%.o匹配foo.o。如果成功%就等于foo。如果不成功，Make就会警告，然后。给foo.o添加依赖文件foo.c(用foo替换了%.c里的%)
# $@--目标文件，$^--所有的依赖文件，$<--第一个依赖文件。每次$< $@ 代表的值就是列表中的
$(C_OBJ_ALL) : %.o: %.c
	$(CC) -c $< -o $@ $(EXTRA_INCS) $(SRC_INCS) $(CFLAGS)
$(CXX_OBJ_ALL) : %.o: %.cpp
	$(CXX) -c $< -o $@ $(EXTRA_INCS) $(SRC_INCS) $(CXXFLAGS)
$(CC_OBJ_ALL) : %.o: %.cc
	$(CXX) -c $< -o $@ $(EXTRA_INCS) $(SRC_INCS) $(CXXFLAGS)

$(TARGET_NAME): $(C_OBJ_ALL) $(CXX_OBJ_ALL) $(CC_OBJ_ALL)
	$(CXX) -o $(TARGET_NAME) $(C_OBJ_ALL) $(CXX_OBJ_ALL)  $(CC_OBJ_ALL) $(SRC_LIBS) $(EXTRA_LIBS) $(CROSS_FLAGS)
	$(RM) $(C_OBJ_ALL) $(CXX_OBJ_ALL) $(CC_OBJ_ALL)
	$(CP) $(TARGET_NAME) $(TARGET_DIR)$(TARGET_NAME)


.PHONY: all $(PLATS) clean cleanall echo

install:
	$(CP_STRIP_DEBUG) $(TARGET_NAME) $(TARGET_DIR)$(TARGET_NAME)
	$(CP_ONLY_DEBUG) $(TARGET_NAME) $(TARGET_DIR)$(PDB_NAME)

clean:
	$(RM) $(TARGET_NAME) $(C_OBJ_ALL) $(CXX_OBJ_ALL) $(CC_OBJ_ALL)

cleanall:
	$(RM) $(TARGET_NAME) $(C_OBJ_ALL) $(CXX_OBJ_ALL) $(CC_OBJ_ALL) $(TARGET_DIR)$(TARGET_NAME) $(TARGET_DIR)$(PDB_NAME)

