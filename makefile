all:
	gcc -o server server.c
	gcc -o client client.c 
	gcc -o client1 client1.c 
	gcc -o clientapp clientapp.c
	gcc -o client1app client1app.c
	gcc -o serverapp serverapp.c
clean:
	rm server client client1 serverapp clientapp client1app
unbind:
	sudo fuser -k 2024/tcp
	sudo fuser -k 2026/tcp
	sudo fuser -k 2028/tcp