CC = /usr/bin/g++
CFLAGS = -fdiagnostics-color=always -m64 -DRTI_UNIX -DRTI_LINUX -DRTI_64BIT -std=c++17 -g
INCLUDES = -I$(HOME)/../../usr/include/nlohmann \
		   -I$(HOME)/rti_connext_dds-6.1.2/include \
           -I$(HOME)/rti_connext_dds-6.1.2/include/ndds \
           -I$(HOME)/rti_connext_dds-6.1.2/include/ndds/hpp \
		   -I$(ROOT)/usr/local/include/liveMedia \
		   -I$(ROOT)/usr/local/include/BasicUsageEnvironment \
		   -I$(ROOT)/usr/local/include/groupsock \
		   -I$(ROOT)/usr/local/include/UsageEnvironment 
LIBPATH = -L$(HOME)/rti_connext_dds-6.1.2/lib/x64Linux4gcc7.3.0 \
		  -L$(ROOT)/lib/x86_64-linux-gnu
LIBS = -lpqxx -lpq -lnddscpp2 -lnddsc -lnddscore -ldl -lm -lpthread -lstdc++fs -lliveMedia -lgroupsock -lssl -lcrypto 

# Source Files and Object Files
SOURCES = $(wildcard *.cpp)
OBJECTS = $(addprefix build/, $(SOURCES:.cpp=.o))
LIVE_DIR = ../Live555/liveMedia

# Live555
LIVE_INCLUDE = -I$(LIVE_DIR)/include
LIVE_LIB = -L$(LIVE_DIR)/lib -lliveMedia -lBasicUsageEnvironment -lUsageEnvironment -lgroupsock \
		   -L$(LIVE_DIR)/../testProgs

# Executable Output
EXECUTABLE = ./build/streamingagent

# Default Rule
all: $(EXECUTABLE)

# Rule for making the executable
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $(INCLUDES) $(LIBPATH) $(OBJECTS) $(LIBS) $(LIVE_LIB) -o $@

# Rule for making object files
build/%.o: %.cpp
	$(CC) $(CFLAGS) $(INCLUDES) $(LIVE_INCLUDE) -c $< -o $@

# Rule for cleaning the build
clean:
	rm -f build/* $(EXECUTABLE)
