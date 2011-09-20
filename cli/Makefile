GCC=gcc
RM=rm -f
CP=cp -f

BIN=h3c_ouc
OBJS=h3c_ouc.o authenticate.o
CFLAGS=-lpcap -lgcrypt

$(BIN): $(OBJS)
	$(GCC) $(CFLAGS) $(OBJS) -o  $@
	$(CP) $@ /usr/local/bin

%.o: %.c
	$(GCC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	$(RM) $(OBJS) $(BIN)
