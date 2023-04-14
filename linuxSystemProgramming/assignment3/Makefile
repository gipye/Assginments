CC = gcc

SDUP = ssu_sfinder

all : $(SDUP)

$(SDUP) : ssu_sfinder.c
	 $(CC) -o $@ $^ -lcrypto -lpthread

clean :
	rm -rf *.o
	rm -rf $(SUDP)
