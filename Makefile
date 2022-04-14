INCLUDES = 
CFLAGS = -g -Wall -fPIC $(INCLUDES)
LDFLAGS = -shared -warn-once -fPIC
OBJ = mbtest.o ModbusTCP.o TCP_util.o
LOBJ = modbus.o ModbusTCP.o TCP_util.o

all: libmodbus.so

mbtest: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

libmodbus.so: $(LOBJ);
	ld $(LDFLAGS) -o libmodbus.so $(LOBJ)

.c.o:
	$(CC) $(CFLAGS) -c $*.c

clean:
	$(RM) *.o *~
