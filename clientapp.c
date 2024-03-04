#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
char *int_to_char(int n, char *nr)
{
	int i = 0, p = 1;
	int cn = n;
	while (n)
	{
		p *= 10;
		n /= 10;
	}
	p /= 10;
	while (cn > 9)
	{
		nr[i++] = cn / p + '0';
		cn = cn % p;
		p /= 10;
	}
	nr[i++] = cn + '0';
	nr[i] = '\0';
	return nr;
}
int main()
{
	int sdC, sdS;
	int PORTC = 2026;
	int PORTS = 2030;
	char adresa[50] = "";
	int nr_clients = 0;
	struct sockaddr_in sapp;
	struct sockaddr_in capp;
	struct sockaddr_in from;

	if ((sdC = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("Eroare la socket capp().\n");
		return errno;
	}
	if ((sdS = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("Eroare la socket capp().\n");
		return errno;
	}

	bzero(&sapp, sizeof(sapp));
	bzero(&capp, sizeof(capp));
	bzero(&from, sizeof(from));

	strcpy(adresa, "127.0.0.1");
	capp.sin_family = AF_INET;
	capp.sin_addr.s_addr = htonl(INADDR_ANY);
	capp.sin_port = htons(PORTC);

	sapp.sin_family = AF_INET;
	sapp.sin_addr.s_addr = inet_addr(adresa);
	sapp.sin_port = htons(PORTS);
	if (bind(sdC, (struct sockaddr *)&capp, sizeof(struct sockaddr)) == -1)
	{
		perror("[clientapp]Eroare la bind().\n");
		return errno;
	}

	if (listen(sdC, 1) == -1)
	{
		perror("[clientapp]Eroare la listen().\n");
		return errno;
	}

	int client;
	int length = sizeof(from);
	client = accept(sdC, (struct sockaddr *)&from, &length);
	printf("Clientapp conectat la client.\n");
	if (connect(sdS, (struct sockaddr *)&sapp, sizeof(struct sockaddr)) == -1)
	{
		perror("[clientapp]Eroare la connect() serverapp.\n");
		return errno;
	}
	printf("Clientapp conectat la serverapp.\n");

	char s[100];
	int num;
	bzero(s, 100);
	do
	{
		printf("ajung aici!\n");
		printf("S-a inregistrat comanda %s \n", s);
		if (num=read(sdC,s,100) < 0)
		{
			perror("[clientapp]Eroare la read() de la client.\n");
			return 0;
		}
		if (write(sdS, s, 100) < 0)
		{
			printf("Problema la scriere in FIFO!");
			return errno;
		}

		char msg[100] = "";
		bzero(msg, 100);
		if (read(sdS, msg, 100) == -1)
		{
			printf("eroare la primirea mesajului1!\n");
			return errno;
		}
		else
		{
			if (strcmp(s, "quit") == 0 && strstr(msg, "succes") != NULL)
			{
				close(sdC);
				close(sdS);
				return 0;
			}
			printf("Am primit mesajul: '%s'\n", msg);
			if (write(sdC, msg, 100) < 0)
			{
				perror("[clientapp]Eroare la write() spre client.\n");
				return errno;
			}
		}
		bzero(s, 100);
	}while(1);
	close(sdC);
	close(sdS);
	return 0;
}