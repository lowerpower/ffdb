
TARGET	= ffdb

all: $(TARGET)

# Which compiler
CC = gcc

# Where are include files kept
INCLUDE = 


# Use this for Linux client software
CDEFINE = -DLINUX 

# Options for development
CFLAGS		= $(INCLUDE) $(CDEFINE) -g -Wall -Os 
LDFLAGS		=
LINKFLAGS	= -o
#OSXLINKFLAGS	= -arch i386 -arch ppc

#LIBS		=
#LIBS		= ../miniupnp_lib/libminiupnpc.a

# Options for release
# CFLAGS = -O -Wall -ansi

#Dependancies for all
DEPENDALL	= 
#config.h mytypes.h debug.h

#Objects
OBJ	= ffdb.o 

$(TARGET): $(OBJ) 
	$(CC) $(LINKFLAGS) $(TARGET) $(OBJ) $(LIBS)

clean:
	-rm *.o $(MYLIB)
	 rm -f *.map
	 rm -f $(TARGET)


