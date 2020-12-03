sinclude .crosscompile

APP1 = xmt
APP2 = rcv
APP3 = xmp
APP4 = pth

BIN = $(APP1) $(APP2) $(APP3) $(APP4)

CC = gcc
CFLAGS = -Wall

all: $(BIN)

%: %.c
	$(CROSS_COMPILE)gcc $(CFLAGS) $< -o $@ -lpthread

$(APP1): $(APP1).c
$(APP2): $(APP2).c
$(APP3): $(APP3).c
$(APP4): $(APP4).c

install: all
	for f in install_* ; \
	do if [ -L "$$f" ] ;then \
		cp -fv $(BIN) $$f/opt/bin; \
	fi; done	

clean:
	rm -f $(BIN) *~ $(OBJS)

re:
	make clean
	make all

