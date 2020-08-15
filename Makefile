# Sources
SRC_DIR = .
OBJS = $(foreach dir,$(SRC_DIR),$(subst .c,.o,$(wildcard $(dir)/*.c))) $(foreach dir,$(SRC_DIR),$(subst .cpp,.o,$(wildcard $(dir)/*.cpp)))

# Compiler Settings
OUTPUT = nrooooooo
CXXFLAGS = -Wall -g -I. -std=c++17 -fsanitize=address -I /path/to/unicorn
CFLAGS = -I. -std=gnu11 -fsanitize=address
# If this fails, use "-pthread" instead of "-lpthread"
#LIBS = -lasan -lpthread -lunicorn -lstdc++fs
LDFLAGS = -L/path/to/unicorn/build
LIBS = -lasan -pthread -lunicorn -lstdc++fs # -liberty
CC = gcc
# Note: requires a g++ that supports the -std=c++17 flag, if g++ in your path doesn't support
#       C++17, update your g++ and possibly edit this to match the right version of g++
# Example:
# CXX = g++8
#CXX = g++
CXX = g++-8
ifeq ($(OS),Windows_NT)
    #Windows Build CFG
    CFLAGS += 
    LIBS += 
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Darwin)
        # OS X
        CFLAGS +=
        LIBS += 
    else
        # Linux
        CFLAGS += 
        CXXFLAGS += 
        LIBS += 
    endif
endif

main: $(OBJS)
	$(CXX) -o $(OUTPUT) $(OBJS) $(LDFLAGS) $(LIBS) 

clean:
	rm -rf $(OUTPUT) $(OUTPUT).exe $(OBJS)
