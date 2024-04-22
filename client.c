#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP> <Port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("Début programme\n");

    // Création du socket client
    int dS = socket(PF_INET, SOCK_STREAM, 0);
    if (dS == -1) {
        perror("Erreur lors de la création du socket");
        exit(EXIT_FAILURE);
    }

    // Configuration de l'adresse et du port du serveur
    struct sockaddr_in aS;
    aS.sin_family = AF_INET;
    inet_pton(AF_INET, argv[1], &(aS.sin_addr));
    aS.sin_port = htons(atoi(argv[2]));
    socklen_t lgA = sizeof(struct sockaddr_in);

    // Connexion au serveur
    if (connect(dS, (struct sockaddr *) &aS, lgA) == -1) {
        perror("Erreur lors de la connexion au serveur");
        exit(EXIT_FAILURE);
    }

    printf("Connecté au serveur\n");

    char message[1024];
    while (1) {
        // Demande de saisie d'un message à envoyer au serveur
        printf("Entrez votre message : ");
        fgets(message, sizeof(message), stdin);

        // Suppression du caractère de nouvelle ligne de la saisie
        message[strcspn(message, "\n")] = 0;

        // Envoi du message au serveur
        send(dS, message, strlen(message), 0);

        // Vérification si le message est "fin", si oui, terminer la connexion
        if (strcmp(message, "fin") == 0) {
            printf("Envoi du message 'fin', terminant la connexion.\n");
            break;
        }

        // Réception de la réponse du serveur
        recv(dS, message, sizeof(message), 0);
        printf("Réponse reçue du serveur : %s\n", message);

        // Vérification si le message reçu est "fin", si oui, terminer la connexion
        if (strcmp(message, "fin") == 0) {
            printf("Le serveur a envoyé 'fin', terminant la connexion.\n");
            break;
        }
    }

    // Fermeture de la connexion avec le serveur
    shutdown(dS, 2);
    printf("Connexion fermée\n");
    return 0;
}
