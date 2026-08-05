CC=gcc
TARGET = ../../bin/sucorrpilot_omp

SRCS=main_omp.c sucorrpilot.c

CFLAGS= -fopenmp -I$(CWPROOT)/include

LDFLAGS= -L$(CWPROOT)/lib
LIBS= -lsu -lpar -lcwp -lm 

all: $(TARGET)

$(TARGET): $(SRCS)
	@mkdir -p ../../bin
	$(CC) $(SRCS) -o $(TARGET) $(CFLAGS) $(LDFLAGS) $(LIBS)

clean:
	rm -f $(TARGET)
