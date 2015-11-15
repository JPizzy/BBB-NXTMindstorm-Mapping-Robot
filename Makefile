OUTFILE = nxtProject
OUTDIR = $(HOME)/cmpt433/public/myApps
CROSS_COMPILE = arm-linux-gnueabi-
CC_C = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -g -std=c99 -D _POSIX_C_SOURCE=200809L -Werror

all:
	$(CC_C) $(CFLAGS) bt_demo.c -o $(OUTDIR)/$(OUTFILE) -lbluetooth
	
#$(CC_C) $(CFLAGS) bt_demo.c -o $(OUTDIR)/$(OUTFILE) -lm -lbluetooth -L/usr/lib -v -Wl,-verbose
