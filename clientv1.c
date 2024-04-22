#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include<unistd.h>

#define  MAX_LEN 1024

int main(int argc, char *argv[]) {

  printf("Début programme\n");
  int dS = socket(PF_INET, SOCK_STREAM, 0);
  printf("Socket Créé\n");

  struct sockaddr_in aS;
  aS.sin_family = AF_INET;
  inet_pton(AF_INET,argv[1],&(aS.sin_addr)) ;
  aS.sin_port = htons(atoi(argv[2])) ;
  socklen_t lgA = sizeof(struct sockaddr_in) ;
  connect(dS, (struct sockaddr *) &aS, lgA) ;
  printf("Socket Connecté\n");
  char recep [MAX_LEN] ;
  char envoie [MAX_LEN] ;
  while (strcmp(recep, "fin\n")!=0 && strcmp(envoie, "fin\n")!=0) {
  printf("Veullez tapez votre message:");
  fgets(envoie,MAX_LEN,stdin);
  printf("\n");
  send(dS, envoie, strlen(envoie) , 0) ;
  printf("Message Envoyé \n");
  
  recv(dS, &recep, sizeof(recep), 0) ;
  printf("Message reçu : %s\n", recep) ;
  }
  shutdown(dS,2) ;
  printf("Fin du programme\n");
}