CROSS=arm-linux-


all: main.c display.c rs485.c misc.c codec.c ulist.c
	$(CROSS)gcc  -o carman main.c display.c rs485.c misc.c codec.c ulist.c

dump: dumplist.c
	$(CROSS)gcc -o dumplist dumplist.c


#	$(CROSS)strip carman
clean:
	@rm -vf carman *.o *~
