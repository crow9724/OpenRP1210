
# CXX = g++
CPPFLAGS = -DOpenRP1210Export
CFLAGS = -Wall -fPIC -g -std=gnu11
LDFLAGS = 
LDLIBS =

SRC_DIR = ../../lib/src
OBJ_DIR = obj
BIN_DIR = bin
INC_DIR = ../../lib/inc

ifeq ($(PREFIX),)
	PREFIX := /usr/lib
endif

LIBNAME := OpenRP1210
SOURCES := $(filter-out $(SRC_DIR)/platform/win, \
	$(wildcard $(SRC_DIR)/*.c)) \
	$(wildcard $(SRC_DIR)/common/*.c) \
	$(wildcard $(SRC_DIR)/util/*.c) \
	$(wildcard $(SRC_DIR)/platform/linux/*.c)

OBJECTS := $(subst $(SRC_DIR), $(OBJ_DIR), $(patsubst %.c, %.o, $(SOURCES)))
DEPENDS := $(subst $(SRC_DIR), $(OBJ_DIR), $(patsubst %.c, %.d, $(SOURCES)))

# DemoApp
DEMOAPP_CPPFLAGS =
DEMOAPP_CXXFLAGS = -Wall -g
DEMOAPP_LDFLAGS = -L$(PREFIX)/x86_x64-linux-gnu
DEMOAPP_LDLIBS = -l$(LIBNAME)

DEMOAPP_SRC_DIR = ../../demo/src
DEMOAPP_INC_DIR = ../../demo/inc -I$(INC_DIR)
DEMOAPP_OBJ_DIR = obj/demo

DEMOAPP_EXENAME := DemoApp
DEMOAPP_SOURCES := $(wildcard $(DEMOAPP_SRC_DIR)/*.cpp)

DEMOAPP_OBJECTS := $(subst $(DEMOAPP_SRC_DIR), $(DEMOAPP_OBJ_DIR), $(patsubst %.cpp, %.o, $(DEMOAPP_SOURCES)))
DEMOAPP_DEPENDS := $(subst $(DEMOAPP_SRC_DIR), $(DEMOAPP_OBJ_DIR), $(patsubst %.cpp, %.d, $(DEMOAPP_SOURCES)))

.PHONY: all clean
all: $(LIBNAME)

$(LIBNAME): $(OBJECTS) | $(BIN_DIR)
	$(CC) $(LDFLAGS) -shared $^ -o $(BIN_DIR)/lib$@.so $(LDLIBS)

$(DEMOAPP_EXENAME): $(DEMOAPP_OBJECTS) | $(BIN_DIR)
	$(CXX) $(DEMOAPP_LDFLAGS) $^ -o $(BIN_DIR)/$@ $(DEMOAPP_LDLIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c Makefile | $(OBJ_DIR)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CPPFLAGS) -I${INC_DIR} -MMD -MP -c $< -o $@

$(DEMOAPP_OBJ_DIR)/%.o: $(DEMOAPP_SRC_DIR)/%.cpp Makefile | $(DEMOAPP_OBJ_DIR)
	$(CXX) $(DEMOAPP_CXXFLAGS) -I${DEMOAPP_INC_DIR} -MMD -MP -c $< -o $@

$(BIN_DIR) $(OBJ_DIR) $(DEMOAPP_OBJ_DIR):
	mkdir -p $@

install: $(BIN_DIR)/lib$(LIBNAME).so
	install $(BIN_DIR)/lib$(LIBNAME).so $(PREFIX)/x86_64-linux-gnu

clean:
	@$(RM) -rv $(BIN_DIR) $(OBJ_DIR)

-include $(DEPENDS)




