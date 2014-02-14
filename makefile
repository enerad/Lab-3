pipe:  
	gcc pipe.c -o pipe
server:
	gcc server.c -o server
client:
	gcc client.c -o client
clean:
	rm -f pipe client server

