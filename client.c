#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>

extern int errno;

int portS = 2024;
int portC = 2026;
char adresa[100];
int main(int argc, char *argv[])
{
  int sock_serv, sock_capp;
  struct sockaddr_in server, capp;
  char msg[100];
  int serv_conn = 0, need_cod = 0;

  printf("Buna ziua!\n Meniul de comenzi:\n -'login';\n -'Check ID;'\n -'Cod 2FA';\n -'Cod: [cod2FA];'\n -'Check ID: [yes/no]';\n -'Quit'. \n");

  if ((sock_serv = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("Eroare la socket server().\n");
    return errno;
  }
  if ((sock_capp = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("Eroare la socket server().\n");
    return errno;
  }
  strcpy(adresa, "127.0.0.1");
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = inet_addr(adresa);
  server.sin_port = htons(portS);

  capp.sin_family = AF_INET;
  capp.sin_addr.s_addr = inet_addr(adresa);
  capp.sin_port = htons(portC);

  if (connect(sock_capp, (struct sockaddr *)&capp, sizeof(struct sockaddr)) == -1)
  {
    perror("[client]Eroare la connect() clientapp.\n");
    return errno;
  }
  if (connect(sock_serv, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
  {
    perror("[client]Eroare la connect() clientapp.\n");
    return errno;
  }

  bzero(msg, 100);
  strcpy(msg, argv[1]);
  if (write(sock_serv, msg, 100) < 0)
  {
    perror("[client]eroare la scrierea numelui aplicatiei.\n");
    return errno;
  }

  printf("Introduceti o comanda...\n");
  while (fgets(msg, 100, stdin), !feof(stdin))
  {
    msg[strlen(msg) - 1] = '\0';

    if (strcmp(msg, "Need code") == 0 && need_cod == 1)
    {
      if (write(sock_serv, msg, 100) < 0)
      {
        perror("[client]Eroare la write()1''.\n");
        return errno;
      }
      if (read(sock_serv, msg, 100) < 0)
      {
        perror("[client]Eroare la read()1''.\n");
        return errno;
      }
      printf("%s\n", msg);
    }
    else
    {
      if (write(sock_capp, msg, 100) < 0)
      {
        perror("[client]Eroare la write()1.\n");
        return errno;
      }

      if (strcmp(msg, "Check ID") == 0 && serv_conn == 1)
      {
      reincearca:
        bzero(msg, 100);
        if (read(sock_serv, msg, 100) < 0)
        {
          perror("[client]Eroare la read()2'.\n");
          return errno;
        }
        printf("%s\n", msg);
        char rasp[20];
        bzero(rasp, 20);
        if (fgets(rasp, 20, stdin) == NULL)
        {
          perror("[client]Eroare la Yes/No.\n");
          return errno;
        }
        rasp[strlen(rasp) - 1] = '\0';
        if (write(sock_serv, rasp, 20) < 0)
        {
          perror("[client]Eroare la write()3'.\n");
          return errno;
        }
        bzero(msg, 100);
        if (read(sock_serv, msg, 100) < 0)
        {
          perror("[client]Eroare la read()3'.\n");
          return errno;
        }
        printf("%s\n", msg);
        if (strcmp(msg, "Sintaxa confirmarii: 'Yes' / 'No'") == 0)
          goto reincearca;
        serv_conn = 0;
      }

      bzero(msg, 100);
      if (read(sock_capp, msg, 100) < 0)
      {
        perror("[client]Eroare la read()1.\n");
        return errno;
      }
      printf("Mesajul primit: %s\n", msg);

      if (strcmp(msg, "Doriti sa confirmati identitatea sau sa introduceti un cod 2FA?") == 0) // evidenta necesitatii conexiunii cu server-ul
        serv_conn = 1;
      else if (strcmp(msg, "Introduceti codul 2FA...") == 0)
        need_cod = 1;
      else if (strcmp(msg, "login success") == 0)
      {
        printf("ajung aici!\n");
        break;
      }
    }

    bzero(msg, 100);
    printf("Introduceti o comanda...\n");
  }
  close(sock_capp);
  close(sock_serv);
  return 0;
}