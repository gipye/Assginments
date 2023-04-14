ssu_sdup: ssu_sdup.c ssu_help ssu_find-md5 ssu_find-sha1
	gcc -o ssu_sdup ssu_sdup.c

ssu_help: ssu_help.c
	gcc -o ssu_help ssu_help.c

ssu_find-md5: ssu_find-md5.c
	gcc -o ssu_find-md5 ssu_find-md5.c -lcrypto

ssu_find-sha1: ssu_find-sha1.c
	gcc -o ssu_find-sha1 ssu_find-sha1.c -lcrypto

clean:
	rm ssu_sdup
	rm ssu_help
	rm ssu_find-md5
	rm ssu_find-sha1
