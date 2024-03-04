#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>
#include <utmp.h>
#include <sys/wait.h>
#include <ctype.h>
#include <sys/socket.h>
#include <pthread.h>

typedef struct thData
{
    int idThread;
    int cl;
    int sv;
} thData;

static void *treat(void *);
void raspunde(void *);

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

bool cauta_user(char *s)
{
    char temp[100];
    char username[strlen(s) - 8];
    strcpy(username, s + 8);
    FILE *fd = fopen("users.txt", "r");
    if (fd == NULL)
    {
        printf("eroare la deschiderea fisierului users.txt\n");
        exit(1);
    }
    else
    {
        while (fgets(temp, 100, fd) != NULL)
        {
            if (!feof(fd))
                temp[strlen(temp) - 1] = '\0';

            if (strcmp(temp, username) == 0)
            {
                fclose(fd);
                return true;
            }
        }
    }
    printf("eroare la prelucrarea din users.txt\n");
    fclose(fd);
    return false;
}

int main()
{
    int PORTS = 2028;
    int PORTC = 2030;
    char s[300];
    int num, nr_clients=0;
    int sock_serv, sock_capp;
    pthread_t th[100];
    struct sockaddr_in server;
    struct sockaddr_in capp;
    struct sockaddr_in from;
    char smsg[100];
    char adresa[50] = "";

    if ((sock_serv = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Eroare la socket server().\n");
        exit(1);
    }
    if ((sock_capp = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Eroare la socket server().\n");
        exit(1);
    }
    bzero(&server, sizeof(capp));
    bzero(&server, sizeof(server));
    bzero(&from, sizeof(from));

    strcpy(adresa, "127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(adresa);
    server.sin_port = htons(PORTS);

    capp.sin_family = AF_INET;
    capp.sin_addr.s_addr = htonl(INADDR_ANY);
    capp.sin_port = htons(PORTC);

    if (connect(sock_serv, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[client]Eroare la connect() clientapp.\n");
        exit(1);
    }
    printf("Serverapp conectat la server!\n");
    if (bind(sock_capp, (struct sockaddr *)&capp, sizeof(struct sockaddr)) == -1)
    {
        perror("[serverapp]Eroare la bind().\n");
        exit(1);
    }

    int on = 1;
    setsockopt(sock_capp, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    printf("Astept o comanda...\n");

    if (listen(sock_capp, 2) == -1)
    {
        perror("[serverapp]Eroare la listen().\n");
        exit(1);
    }

    while (1)
    {
        int clientapp;
        thData *td;
        int length = sizeof(from);
        printf("[server]Asteptam la portul %d...\n", PORTC);

        if ((clientapp = accept(sock_capp, (struct sockaddr *)&from, &length)) < 0)
        {
            perror("[server]Eroare la accept().\n");
            continue;
        }
        td=(struct thData*)malloc(sizeof(struct thData));	
	    td->idThread=nr_clients++;
	    td->cl=clientapp;
        td->sv=sock_serv;

	    pthread_create(&th[nr_clients], NULL, &treat, td);
    }
    close(sock_serv);
    close(sock_capp);
};
static void *treat(void *arg)
{
    struct thData tdL;
    tdL = *((struct thData *)arg);
    printf("[thread]- %d - Asteptam mesajul...\n", tdL.idThread);
    fflush(stdout);
    pthread_detach(pthread_self());
    raspunde((struct thData *)arg);
    close((intptr_t)arg);
    close(tdL.cl);
    return (NULL);
};

void raspunde(void *arg)
{
    struct CL
    {
        int pas1, pas2;
        char *cod;
    } clientData;
    
    int num; 
    char s[300]="";
    struct thData tdL;
    tdL = *((struct thData *)arg);
    bzero(s,100);
    if ((num = read(tdL.cl, s, 100)) == -1)
    {
        printf("eroare la citirea comenzii\n");
        exit(1);
    }
    else
    {
        printf("Am primit comanda %s\n", s);
        printf("id:%d mess:%s\n",tdL.idThread,s);

        char msg[100]="";
        if (strstr(s, "login : ") != NULL && clientData.pas1 == 0)
        {
            if (cauta_user(s) == true)
            {
                clientData.pas1 = 1;

                bzero(msg, 100);
                strcpy(msg, "Doriti sa confirmati identitatea sau sa introduceti un cod 2FA?");

                if (write(tdL.cl, msg, 100) < 0)
                {
                    printf("eroare la transmiterea mesajului\n");
                    exit(1);
                }
                printf("%s\n", msg);
            }
            else
            {
                bzero(msg, 100);
                strcpy(msg, "Username or password wrong");
                if (write(tdL.cl, msg, 100) < 0)
                {
                    printf("eroare la transmiterea mesajului\n");
                    exit(1);
                }
                printf("%s\n", msg);
            }
        }
        else if (strcmp(s, "Check ID") == 0 && clientData.pas2 == 0 && clientData.pas1 == 1)
        {
            printf("trimitem notificare\n");
            bzero(msg, 100);
            strcpy(msg, "Check ID/");
            char nr[20]="";
            bzero(nr,20);
            strcat(msg, int_to_char(tdL.idThread,nr));
            if (write(tdL.sv, msg, 100) < 0)
            {
                perror("eroare la write() spre server.\n");
                exit(1);
            }
            bzero(msg, 100);
            if (read(tdL.sv, msg, 100) < 0)
            {
                perror("eroare la read() dinspre server.\n");
                exit(1);
            }
            printf("Raspunsul primit: %s\n", msg);
            if (strcmp(msg, "Respingere") == 0)
            {
                bzero(msg, 100);
                strcpy(msg, "login failed");
                if (write(tdL.cl, msg, 100) < 0)
                {
                    perror("eroare la write() fifo.\n");
                    exit(1);
                }
                printf("%s\n", msg);
            }
            else if (strcmp(msg, "Aprobare") == 0)
            {
                clientData.pas2 = 1;
                bzero(msg, 100);
                strcpy(msg, "login success");
                if (write(tdL.cl, msg, 100) < 0)
                {
                    perror("eroare la write() fifo.\n");
                    exit(1);
                }
                printf("%s\n", msg);
            }
        }
        else if (strcmp(s, "Cod 2FA") == 0 && clientData.pas2 == 0 && clientData.pas1 == 1)
        {
            bzero(msg, 100);
            strcpy(msg, "Introduceti codul 2FA...");
            if (write(tdL.cl, msg, 100) < 0)
            {
                printf("eroare la scrierea mesajului eroare\n");
                exit(1);
            }
            printf("%s\n", msg);

            bzero(msg, 100);
            strcpy(msg, "Cod 2FA/");
            char nr[20]="";
            bzero(nr,20);
            strcat(msg, int_to_char(tdL.idThread,nr));

            if (write(tdL.sv, msg, 100) < 0)
            {
                printf("eroare la scrierea mesajului eroare\n");
                exit(1);
            }

            bzero(msg, 100);
            if (read(tdL.sv, msg, 100) < 0)
            {
                perror("eroare la citire cod din server.\n");
                exit(1);
            }
            strcpy(clientData.cod, msg);
            printf("Am primit codul din mesajul: %s\n", clientData.cod);
        }
        else if (strstr(s, "Cod : ") && clientData.pas2 == 0 && clientData.pas1 == 1)
        {
            if (strcmp(s + 6, clientData.cod) == 0)
            {
                clientData.pas2 = 1;
                bzero(msg, 100);
                strcpy(msg, "login success");
                if (write(tdL.cl, msg, 100) < 0)
                {
                    perror("eroare la write()2 in fifo.\n");
                    exit(1);
                }
                printf("%s\n", msg);
            }
            else
            {
                bzero(msg, 100);
                strcpy(msg, "login failed");
                if (write(tdL.cl, msg, 100) < 0)
                {
                    perror("eroare la write()2 in fifo.\n");
                    exit(1);
                }
                printf("%s\n", msg);
            }
        }
        else
        {
            if (write(tdL.cl, "EROARE", strlen("EROARE")) == -1)
            {
                printf("eroare la scrierea mesajului eroare\n");
                exit(1);
            }
            printf("%s\n", "EROARE");
        }
    }
}