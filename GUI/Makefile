DESUDIR = /usr/bin/
CONFDIR = /usr/share/gh3c/
DSKTDIR = /usr/share/applications/
GCC=gcc
RM=rm -f
INSTALL=install

BIN=h3c_ouc
OBJS=h3c_ouc.o authenticate.o
CFLAGS=-lpcap -lgcrypt

$(BIN): $(OBJS)
	$(GCC) $(CFLAGS) $(OBJS) -o  $@

%.o: %.c
	$(GCC) $(CFLAGS) -c $<

.PHONY: clean install uninstall

install:
	$(INSTALL) -d $(CONFDIR)
	$(INSTALL) -p -D -m 0755 $(BIN) $(CONFDIR)
	$(INSTALL) -p -D h3c.png $(CONFDIR)
	$(INSTALL) -p -D connected.png $(CONFDIR)
	$(INSTALL) -p -D disconnected.png $(CONFDIR)
	$(INSTALL) -p -D g-h3c_ouc.py $(CONFDIR)
	$(INSTALL) -d $(DESUDIR)
	$(INSTALL) -p -D -m 0755 h3c $(DESUDIR)
	$(INSTALL) -d $(DSKTDIR)
	$(INSTALL) -p -D -m 0755 H3C.desktop $(DSKTDIR)		

uninstall:
	$(RM) $(CONFDIR)$(BIN)
	$(RM) $(CONFDIR)h3c.png
	$(RM) $(CONFDIR)connected.png
	$(RM) $(CONFDIR)disconnected.png
	$(RM) $(CONFDIR)g-h3c_ouc.py
	$(RM) $(DESUDIR)h3c
	$(RM) $(DSKTDIR)H3C.desktop
	
clean:
	$(RM) $(OBJS) $(BIN)

