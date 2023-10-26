CC = /usr/bin/g++
CFLAGS = -fdiagnostics-color=always -m64 -DRTI_UNIX -DRTI_LINUX -DRTI_64BIT -std=c++17 -g
INCLUDES = -I../work/libdatachannel/include/ \
		   -I../work/libdatachannel/deps/json/include \
		   -I$(HOME)/rti_connext_dds-6.1.2/include \
           -I$(HOME)/rti_connext_dds-6.1.2/include/ndds \
           -I$(HOME)/rti_connext_dds-6.1.2/include/ndds/hpp
LIBPATH = -L$(HOME)/rti_connext_dds-6.1.2/lib/x64Linux4gcc7.3.0
LIBS = -lpqxx -ldatachannel -lpq -lnddscpp2 -lnddsc -lnddscore -ldl -lm -lpthread

# Source Files and Object Files
SOURCES = $(wildcard *.cpp)
OBJECTS = $(addprefix build/, $(SOURCES:.cpp=.o))

# Executable Output
EXECUTABLE = ./build/streamingagent

# Default Rule
all: $(EXECUTABLE)

# Rule for making the executable
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $(INCLUDES) $(LIBPATH) $(OBJECTS) $(LIBS) -o $@

# Rule for making object files
build/%.o: %.cpp
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Rule for cleaning the build
clean:
	rm -f build/* $(EXECUTABLE)
