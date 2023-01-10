
CC = gcc -lpthread 
servermake:  server.c node.c filesManeger.c
 
	$(CC) -o server server.c
	$(CC) -o node node.c
	$(CC) -o filesManeger filesManeger.c

clean :
	rm server node filesManeger *.txt
