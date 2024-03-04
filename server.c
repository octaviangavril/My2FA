#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>

#define PORTS 2028
#define PORT 2024

extern int errno;

struct
{ // structura de stocare a aplicatiilor
	char *name;
	int fd;
	struct cl
	{
		int fd;
		char *cod;
	} *clients;
	int nr_clients;
} app; // momentan avem doar o aplicatie dar pentru mai multe aplicatii cream un vector '[...]'

typedef struct thData
{
	int idThread; // id-ul thread-ului tinut in evidenta de acest program
	int cl;		  // descriptorul intors de accept
	int server;
} thData;

static void *treat(void *);
void raspunde(void *);

char *code_generator(char *code)
{
	srand(time(NULL));
	char alphabet[62] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
						 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
						 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
						 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
						 '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
	for (int i = 0; i < 6; i++)
		code[i] = alphabet[rand() % 62];
	code[6] = '\0';
	return code;
}
void init_app(char *name, int fd)
{
	app.name = (char *)(malloc(strlen(name) * sizeof(char) + 1));
	strcpy(app.name, name);
	app.fd = fd;
	app.nr_clients = 0;
	app.clients = (struct cl *)(malloc(10 * sizeof(struct cl)));
}
void new_client(int fd)
{
	app.clients = (struct cl *)(realloc(app.clients, (app.nr_clients + 10) * sizeof(struct cl)));
	app.clients[app.nr_clients].fd = fd;
	app.clients[app.nr_clients].cod = (char *)(malloc(6 * sizeof(char) + 1));
	strcpy(app.clients[app.nr_clients].cod, code_generator(app.clients[app.nr_clients].cod));
	app.nr_clients++;
}
void erase_app()
{
	free(app.name);
	for (int i = 0; i < app.nr_clients; i++)
	{
		app.clients[i].fd = -1;
		free(app.clients[i].cod);
	}
	free(app.clients);
	app.fd = -1;
	app.nr_clients = 0;
}
char *extract_mess(char *s)
{
	int i;
	char temp[strlen(s)];
	for (i = 0; i < strlen(s); i++)
	{
		if (s[i] != '/')
			temp[i] = s[i];
		else
			break;
	}
	temp[i] = '\0';
	strcpy(s, temp);
	return s;
}

char *extract_id(char *s)
{
	return strchr(s, '/') + 1;
}
int main()
{
	struct sockaddr_in server1, server2;
	struct sockaddr_in fromclient, fromserver;
	char msg[100];
	char msgrasp[100] = " ";
	int sd1, sd2;
	pthread_t th[100];
	if ((sd1 = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("[server]Eroare la socket()1.\n");
		return errno;
	}
	if ((sd2 = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("[server]Eroare la socket()2.\n");
		return errno;
	}

	int on = 1;
	setsockopt(sd1, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	setsockopt(sd2, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	bzero(&server1, sizeof(server1));
	bzero(&server2, sizeof(server2));
	bzero(&fromclient, sizeof(fromclient));
	bzero(&fromserver, sizeof(fromserver));

	server1.sin_family = AF_INET;
	server1.sin_addr.s_addr = htonl(INADDR_ANY);
	server1.sin_port = htons(PORT);

	server2.sin_family = AF_INET;
	server2.sin_addr.s_addr = htonl(INADDR_ANY);
	server2.sin_port = htons(PORTS);

	if (bind(sd1, (struct sockaddr *)&server1, sizeof(struct sockaddr)) == -1)
	{
		perror("[server]Eroare la bind().\n");
		return errno;
	}
	if (bind(sd2, (struct sockaddr *)&server2, sizeof(struct sockaddr)) == -1)
	{
		perror("[server]Eroare la bind().\n");
		return errno;
	}

	if (listen(sd1, 1) == -1)
	{
		perror("[server]Eroare la listen().\n");
		return errno;
	}
	if (listen(sd2, 1) == -1)
	{
		perror("[server]Eroare la listen().\n");
		return errno;
	}

	int serverapp;
	int length2 = sizeof(fromserver);
	serverapp = accept(sd2, (struct sockaddr *)&fromserver, &length2);

	if (serverapp < 0)
	{
		perror("[client]eroare la accept serverapp.\n");
		return errno;
	}

	init_app("yahoo", serverapp); // am umplut structura app;

	while (1)
	{
		int client;
		thData *td;
		int length1 = sizeof(fromclient);

		printf("[server]Asteptam la portul %d si la portul %d...\n", PORT, PORTS);
		fflush(stdout);

		client = accept(sd1, (struct sockaddr *)&fromclient, &length1);

		if (client < 0)
		{
			perror("[server]Eroare la accept() la client.\n");
			continue;
		}

		char msg[100] = "";
		bzero(msg, 100);
		if (read(client, msg, 100) < 0)
		{
			perror("[server]eroare la read() din client.\n");
			return errno;
		}
		if (strcmp(msg, app.name) == 0)
		{
			bzero(msg, 100);			
			td = (struct thData *)malloc(sizeof(struct thData));
			td->idThread = app.nr_clients;
			new_client(client);
			td->cl = client;
			td->server = app.fd;
			pthread_create(&th[app.nr_clients], NULL, &treat, td);
			printf("S-a conectat un nou client cu fd=%d si cod=%s\n", app.clients[app.nr_clients - 1].fd, app.clients[app.nr_clients - 1].cod);
		}
		else
		{
			printf("[server]Nu avem in gestiune aplicatia %s\n",msg);
			exit(1);
		}
	}
	close(serverapp);
};
static void *treat(void *arg)
{
	struct thData tdL;
	tdL = *((struct thData *)arg);
	printf("[thread]- %d - Asteptam mesajul...\n", tdL.idThread);
	fflush(stdout);
	pthread_detach(pthread_self());
	raspunde((struct thData *)arg);
	/* am terminat cu acest client, inchidem conexiunea */
	close((intptr_t)arg);
	return (NULL);
};

void raspunde(void *arg)
{
	struct thData tdL;
	tdL = *((struct thData *)arg);

	char msg[100] = "";
	bzero(msg, 100);
	if (read(tdL.server, msg, 100) < 0)
	{
		perror("[server]eroare la read() din serverapp.\n");
		return errno;
	}

	char id[10] = "";
	strcpy(id, extract_id(msg));
	strcpy(msg, extract_mess(msg));
	printf("id:%s mess:%s\n", id, msg);
	printf("Server-ul il va contacta pe clientul cu id-ul = %s\n", id);

	if (strcmp(msg, "Check ID") == 0)
	{
		printf("ajung aici1\n");
		char notif[100] = "", rasp[20] = "";
		bzero(notif, 100);
		strcpy(notif, "[Notificare] Ai solicitat confirmarea identitatii pentru contul '");
		strcat(notif, id);
		strcat(notif, "'?-[Yes/No]");
		printf("ii scriem lui %d\n", app.clients[atoi(id)].fd);
	reincearca:
		if (write(app.clients[atoi(id)].fd, notif, 100) < 0)
		{
			perror("[server]Eroare la write() spre client.1\n");
			return errno;
		}
		bzero(rasp, 20);
		if (read(app.clients[atoi(id)].fd, rasp, 20) < 0)
		{
			perror("[server]Eroare la read() spre client.2\n");
			return errno;
		}
		if (strcmp(rasp, "Yes") == 0)
		{
			bzero(msg, 100);
			strcpy(msg, "Aprobare");
			if (write(tdL.server, msg, 100) < 0)
			{
				perror("[server]Eroare la write() spre client.4\n");
				return errno;
			}

			bzero(msg, 100);
			strcpy(msg, "Multumim pentru confirmare!");
			if (write(app.clients[atoi(id)].fd, msg, 100) < 0)
			{
				perror("[server]Eroare la write() spre client.4\n");
				return errno;
			}
		}
		else if (strcmp(rasp, "No") == 0)
		{
			bzero(msg, 100);
			strcpy(msg, "Respingere");
			if (write(tdL.server, msg, 100) < 0)
			{
				perror("[server]Eroare la write() spre client.6\n");
				return errno;
			}

			bzero(msg, 100);
			strcpy(msg, "Vom respinge incercarea de autentificare");
			if (write(app.clients[atoi(id)].fd, msg, 100) < 0)
			{
				perror("[server]Eroare la write() spre client.4\n");
				return errno;
			}
		}
		else
		{
			bzero(msg, 100);
			strcpy(msg, "Sintaxa confirmarii: 'Yes' / 'No'");
			if (write(app.clients[atoi(id)].fd, msg, 100) < 0)
			{
				perror("[server]Eroare la write() spre client.6\n");
				return errno;
			}
			goto reincearca;
		}
		printf("Am primit raspunsul %s\n", rasp);
	}
	else if (strcmp(msg, "Cod 2FA") == 0)
	{
		printf("citim de la %d\n", app.clients[atoi(id)].fd);
		bzero(msg, 100);
		if (read(app.clients[atoi(id)].fd, msg, 100) < 0)
		{
			perror("[server]Eroare la read() spre client.2\n");
			return errno;
		}
		bzero(msg, 100);
		strcpy(msg, app.clients[atoi(id)].cod);
		printf("Am generat codul %s\n", msg);
		if (write(tdL.server, msg, 100) < 0)
		{
			perror("[server]Eroare la write() spre serverapp.2\n");
			return errno;
		}
		if (write(app.clients[atoi(id)].fd, msg, 100) < 0)
		{
			perror("[server]Eroare la write() spre client.2\n");
			return errno;
		}
	}
}
