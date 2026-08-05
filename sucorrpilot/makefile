CC=gcc
TARGET = ../../bin/sucorrpilot

SRCS=main.c sucorrpilot.c

CFLAGS= -I/$(CWPROOT)/include

LDFLAGS= -L/$(CWPROOT)/lib
LIBS= -lsu -lpar -lcwp -lm 

all: $(TARGET)

$(TARGET): $(SRCS)
	@mkdir -p ../../bin
	$(CC) $(SRCS) -o $(TARGET) $(CFLAGS) $(LDFLAGS) $(LIBS)

clean:
	rm -f $(TARGET)
