all:
	gcc -g emailclient.c -o emailclient
	gcc -g emailserver.c -o emailserver
	mkdir mail_root
	mkdir mail_root/users
	touch mail_root/users.txt
clean:
	rm emailclient emailserver -rf mail_root
