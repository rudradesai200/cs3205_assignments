all:
	gcc -g -w emailclient.c -o emailclient.o
	gcc -g -w emailserver.c -o emailserver.o
	mkdir mail_root
	mkdir mail_root/users
	touch mail_root/users.txt
clean:
	rm emailclient.o emailserver.o -rf mail_root
