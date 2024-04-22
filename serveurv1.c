#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include<unistd.h>

#define  MAX_LEN  1024

int main(int argc, char *argv[]) {

  printf("Début programme\n");

  int dS = socket(PF_INET, SOCK_STREAM, 0);
  printf("Socket Créé\n");


  struct sockaddr_in ad;
  ad.sin_family = AF_INET;
  ad.sin_addr.s_addr = INADDR_ANY ;
  ad.sin_port = htons(atoi(argv[1])) ;
  if (bind(dS, (struct sockaddr*)&ad, sizeof(ad)) == -1) {
    printf("Erreur \n");
    exit(1);
  } 
  printf("Socket Nommé\n");

  listen(dS, 7) ;
  printf("Mode écoute\n");
  char envoie [MAX_LEN] ;
  char recep [MAX_LEN] ;
  while (1) {
    printf("En attente d'une connexion\n");
    struct sockaddr_in aC ;
    socklen_t lg = sizeof(struct sockaddr_in) ;
    int dSC = accept(dS, (struct sockaddr*) &aC,&lg) ;
     printf("Client Connecté\n");
    while (strcmp(recep, "fin\n")!=0 && strcmp(envoie, "fin\n")!=0) {
      ssize_t indice = recv(dSC, recep, sizeof(recep)-1, 0) ;
      recep[indice] = '\0' ;
      printf("Message reçu : %s\n", recep) ;
      printf("Veullez tapez votre message:");
      fgets(envoie,MAX_LEN,stdin);
      printf("\n");
      send(dSC, &envoie, sizeof(envoie), 0) ; 
      printf("envoie: %s\n", envoie);
      printf("recepe: %s\n", recep);
    }
    recep[0] = '\0';
    envoie[0] = '\0';
    shutdown(dSC, 2) ; 
    printf("Connexion avec le client terminée\n");
  }
  shutdown(dS, 2) ; 
  printf("Fin du programme\n");
}