client: udp_client.c
				gcc udp_client.c -o client.out

server: udp_server.c
				gcc -pthread udp_server.c -o server.out