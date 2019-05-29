# Change to executable name.

PROGUSR := user
PROGSRV := server
INC := 
CC := gcc
# Add whatever compiler flags you want.
CXXFLAGS := 
CPPFLAGS += -Wall -pthread -D_REENTRANT $(INC)

# You MUST keep this for auto-dependency generation.
CXXFLAGS += -MMD

# Can change depending on project
LDLIBS := 

# Not sure when you will really need this. Can leave blank usually.
LDFLAGS :=

# Change 'src/' to where ever you hold src files relative to Makefile.
SRCSSRV := $(wildcard ./server_src/*.c)
SRCSUSR := $(wildcard ./user_src/*.c)


# Generate .o and .d filenames for each .cpp file.
# Doesn't generate the ACTUAL files (compiler does).
# Just generates the lists.
OBJSSRV := $(SRCSSRV:.c=.o)
DEPSSRV := $(OBJSSRV:.o=.d)

OBJSUSR := $(SRCSUSR:.c=.o)
DEPSUSR := $(OBJSUSR:.c=.o)

# GNUMake feature, in case you have files called 'all' or 'clean'.
.PHONY: all clean

# Called when you run 'make'. This calls the line below.
all: $(PROGSRV) $(PROGUSR)


# Calls the compiler with flags to link all object files together.
$(PROGSRV): $(OBJSSRV)
	$(CC) $(CPPFLAGS) $(LDLIBS) $(OBJSSRV) -o $(PROGSRV)
	

$(PROGUSR): $(OBJSUSR)
	$(CC) $(CXXFLAGS) $(LDLIBS) $(OBJSUSR) -o $(PROGUSR)
	

# Includes the dependency lists (.d files).
-include $(DEPS)

# Removes exectuable, object files, and dependency files.
clean:
	rm -f $(PROGSRV)
	rm -f $(PROGUSR)
	rm -f $(DEPSUSR) $(DEPSSRV) $(OBJSSRV) $(OBJSUSR)
	rm -f app.xml

run: all