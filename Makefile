OBJECTS = flac123.o

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

all: flac123

clean:
	rm -f flac123 *.o core

install:
	$(INSTALL) -m 755 flac123 $(PREFIX)/bin/

flac123: $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS) $(AO_LIBS) $(FLAC_LIBS) $(POPT_LIBS)
