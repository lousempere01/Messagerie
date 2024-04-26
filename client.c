#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>

#define MAX_LEN 50
char msg[MAX_LEN];
char input[MAX_LEN]; 

void *readMessage(void *arg) {
    int dS = *(int *)arg;
    int nb_recv;

    while(1) {

        // Read message
        nb_recv = recv(dS, msg, MAX_LEN, 0);
        if (nb_recv == -1) {
            perror("Erreur lors de la reception du message");
            close(dS);
            exit(EXIT_FAILURE);
        } else if (nb_recv == 0) {
            // Connection closed by client or server
            break;
        }

        // Check if the message is "fin"
        // If it is, close the socket and exit
        if (strcmp(msg, "fin") == 0) {
            printf("Un client a quitté la discussion\n");
        } else {
            printf("Message reçu: %s\n", msg);
        }
    }

    pthread_exit(0);
}

void *writeMessage(void *arg) {
    int dS = *(int *)arg;
    int nb_send;

    while(1) {
        fgets(input, MAX_LEN, stdin); 
        char *pos = strchr(input, '\n');
        *pos = '\0';

        printf("Message envoyé: %s\n", input);
        
        // Send message
        nb_send = send(dS, input, MAX_LEN, 0);
        if (nb_send == -1) {
            perror("Erreur lors de l'envoi du message");
            close(dS);
            exit(EXIT_FAILURE);
        } else if (nb_send == 0) {
            // Connection closed by remote host
            printf("Le serveur a fermé la connexion\n");
            close(dS);
            break;
        }

        // Check if the message is "fin"
        // If it is, close the socket and exit
        if (strcmp(input, "fin") == 0) {
            printf("Vous avez mis fin à la discussion\n");
            close(dS);
            break;
        }
    }

    pthread_exit(0);
}

void handle_sigint(int sig) {
    printf("Pour quitter, veuillez saisir le mot 'fin'.\n");
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Error: You must provide exactly 2 arguments.\n\
                Usage: %s <server_ip> <server_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("Début programme client\n");
    printf("Bienvenue sur la messagerie instantanée !\n");

    int dS = socket(PF_INET, SOCK_STREAM, 0);
    if (dS == -1) {
        perror("Erreur création socket");
        exit(EXIT_FAILURE);
    }

    printf("Socket Créé\n");

    struct sockaddr_in aS;
    aS.sin_family = AF_INET;
    int result = inet_pton(AF_INET, argv[1], &(aS.sin_addr));
    if (result == 0) {
        fprintf(stderr, "Adresse invalide\n");
        exit(EXIT_FAILURE);
    } else if (result == -1) {
        perror("inet_pton");
        exit(EXIT_FAILURE);
    }

    aS.sin_port = htons(atoi(argv[2])) ;
    socklen_t lgA = sizeof(struct sockaddr_in) ;
    if (connect(dS, (struct sockaddr *) &aS, lgA) == -1) {
        perror("Erreur connect client");
        exit(EXIT_FAILURE);
    }

    printf("Socket Connecté\n\n");

    signal(SIGINT, handle_sigint);

    pthread_t readThread;
    pthread_t writeThread;

    if (pthread_create(&readThread, NULL, readMessage, &dS) != 0) {
        perror("Erreur création thread de lecture");
        close(dS);
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&writeThread, NULL, writeMessage, &dS) != 0) {
        perror("Erreur création thread d'écriture");
        close(dS);
        exit(EXIT_FAILURE);
    }

    if (pthread_join(readThread, NULL) != 0) {
        perror("Erreur fermeture thread de lecture");
        close(dS);
        exit(EXIT_FAILURE);
    }

    if (pthread_join(writeThread, NULL) != 0) {
        perror("Erreur fermeture thread d'écriture");
        close(dS);
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}
