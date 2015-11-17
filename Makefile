TARGET = nxtMapper
PUBDIR = $(HOME)/cmpt433/public/myApps
OUTDIR = $(PUBDIR)
CROSS_COMPILE = arm-linux-gnueabi-
CC_C = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -g -std=c99 -D _POSIX_C_SOURCE=200809L -Werror

SOURCES = nxtMapper.c udpListener.c

all: node
	$(CC_C) $(CFLAGS) $(SOURCES) -o $(OUTDIR)/$(TARGET) -lbluetooth -lpthread
	
node:
	mkdir -p $(PUBDIR)/nxt-server-copy/ 
	cp -R server/* $(PUBDIR)/nxt-server-copy/  

clean:
	rm -f $(OUTDIR)/$(TARGET)
