#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <Port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("Début du programme\n");

    // Création du socket serveur
    int dS = socket(PF_INET, SOCK_STREAM, 0);
    if (dS == -1) {
        perror("Erreur lors de la création du socket");
        exit(EXIT_FAILURE);
    }
    printf("Socket Créé\n");

    // Configuration de l'adresse et du port du serveur
    struct sockaddr_in ad;
    ad.sin_family = AF_INET;
    ad.sin_addr.s_addr = INADDR_ANY;
    ad.sin_port = htons(atoi(argv[1]));

    // Association du socket avec l'adresse et le port
    if (bind(dS, (struct sockaddr*)&ad, sizeof(ad)) == -1) {
        perror("Erreur lors du nommage du socket");
        exit(EXIT_FAILURE);
    }
    printf("Socket Nommé\n");

    // Mise en écoute du socket pour les connexions entrantes
    if (listen(dS, 2) == -1) {
        perror("Erreur en mettant le socket en mode écoute");
        exit(EXIT_FAILURE);
    }
    printf("Mode écoute\n");

    // Attente de la connexion de deux clients
    struct sockaddr_in aC;
    socklen_t lg = sizeof(struct sockaddr_in);
    int dSC1 = accept(dS, (struct sockaddr*) &aC, &lg);
    printf("Client 1 Connecté\n");

    int dSC2 = accept(dS, (struct sockaddr*) &aC, &lg);
    printf("Client 2 Connecté\n");

    char message[1024];
    while (1) {
        // Client 1 écrit puis client 2 lit
        recv(dSC1, message, sizeof(message), 0);
        printf("Client 1 a envoyé : %s\n", message);
        send(dSC2, message, strlen(message), 0);

        if (strcmp(message, "fin") == 0) {
            printf("Client 1 a envoyé 'fin', terminant la connexion.\n");
            break;
        }

        // Client 2 écrit puis client 1 lit
        recv(dSC2, message, sizeof(message), 0);
        printf("Client 2 a envoyé : %s\n", message);
        send(dSC1, message, strlen(message), 0);

        if (strcmp(message, "fin") == 0) {
            printf("Client 2 a envoyé 'fin', terminant la connexion.\n");
            break;
        }
    }

    // Fermeture des sockets
    shutdown(dSC1, 2);
    shutdown(dSC2, 2);
    shutdown(dS, 2);
    printf("Fin du programme\n");
    return 0;
}
