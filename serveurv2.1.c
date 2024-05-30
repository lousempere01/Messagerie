#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_CLIENT 20
#define BUFFER_SIZE 1000

struct Client {
    int socket;
    char pseudo[BUFFER_SIZE];
};

struct Message {
    char pseudo[BUFFER_SIZE];
    char contenu[BUFFER_SIZE];
};

pthread_t tab_thread[MAX_CLIENT];
struct Client tab_client[MAX_CLIENT];
struct sockaddr_in tab_adr[MAX_CLIENT];
socklen_t tab_lg[MAX_CLIENT];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

bool is_pseudo_taken(const char *pseudo) {
    for (int i = 0; i < MAX_CLIENT; i++) {
        if (tab_client[i].socket != -1 && strcmp(tab_client[i].pseudo, pseudo) == 0) {
            return true;
        }
    }
    return false;
}

void *broadcast(void *client) {
    struct Client *data = (struct Client *)client;
    int dSC = data->socket;
    struct Message message;
    int nb_recv, nb_send;
    int i;

    while (1) {
        nb_recv = recv(dSC, &message.contenu, BUFFER_SIZE, 0);
        if (nb_recv == -1) {
            perror("Erreur lors de la reception");
            break;
        } else if (nb_recv == 0) {
            printf("Client déconnecté\n");
            break;
        }

        if (strcmp(message.contenu, "/kill") == 0) {
            char messFin[BUFFER_SIZE] = "Le serveur est down.";
            for (i = 0; i < MAX_CLIENT; i++) {
                if (tab_client[i].socket != -1) {
                    nb_send = send(tab_client[i].socket, &messFin, BUFFER_SIZE, 0);
                    if (nb_send == -1) {
                        perror("Erreur lors de l'envoi");
                    }
                }
            }
            break;
        } else if (strcmp(message.contenu, "/fin") == 0) {
            printf("Fin de la discussion pour client : %s\n", data->pseudo);
            break;
        } else if (strncmp(message.contenu, "/mp", 3) == 0) {
            char destinataire[BUFFER_SIZE];
            char contenu[BUFFER_SIZE];
            sscanf(message.contenu, "/mp %s %[^\n]", destinataire, contenu);
            bool destinataire_trouve = false;
            for (i = 0; i < MAX_CLIENT; i++) {
                if (strcmp(tab_client[i].pseudo, destinataire) == 0) {
                    nb_send = send(tab_client[i].socket, contenu, BUFFER_SIZE, 0);
                    if (nb_send == -1) {
                        perror("Erreur lors de l'envoi");
                    }
                    destinataire_trouve = true;
                    break;
                }
            }
            if (!destinataire_trouve) {
                char erreur_message[BUFFER_SIZE];
                snprintf(erreur_message, BUFFER_SIZE, "Le destinataire '%s' n'existe pas.", destinataire);
                nb_send = send(dSC, erreur_message, strlen(erreur_message), 0);
                if (nb_send == -1) {
                    perror("Erreur lors de l'envoi");
                }
            }
        } else {
            printf("%s : %s\n", data->pseudo, message.contenu);
            for (i = 0; i < MAX_CLIENT; i++) {
                if (tab_client[i].socket != -1 && tab_client[i].socket != dSC) {
                    nb_send = send(tab_client[i].socket, &message.contenu, BUFFER_SIZE, 0);
                    if (nb_send == -1) {
                        perror("Erreur lors de l'envoi");
                    }
                }
            }
        }
    }

    if (strcmp(message.contenu, "/kill") == 0) {
        for (i = 0; i < MAX_CLIENT; i++) {
            if (tab_client[i].socket != -1 && tab_client[i].socket != dSC) {
                if (close(tab_client[i].socket) == -1) {
                    perror("Erreur lors de la fermeture du descripteur de fichier");
                }
                pthread_mutex_lock(&mutex);
                tab_client[i].socket = -1;
                pthread_mutex_unlock(&mutex);
            }
        }
    }

    if (close(dSC) == -1) {
        perror("Erreur lors de la fermeture du descripteur de fichier");
    }

    pthread_mutex_lock(&mutex);
    data->socket = -1;
    pthread_mutex_unlock(&mutex);

    pthread_exit(0);
}

void *connexion(void *dServeur) {
    int dS = *(int *)dServeur;
    char pseudo[BUFFER_SIZE];

    while (1) {
        for (int i = 0; i < MAX_CLIENT; i++) {
            if (tab_client[i].socket == -1) {
                tab_lg[i] = sizeof(struct sockaddr_in);
                int client_socket = accept(dS, (struct sockaddr *)&tab_adr[i], &tab_lg[i]);
                if (client_socket == -1) {
                    perror("Erreur lors de la connexion avec le client");
                    continue;
                }

                // Recevoir le pseudo du client
                int recv_ret = recv(client_socket, pseudo, sizeof(pseudo), 0);
                if (recv_ret <= 0) {
                    perror("Erreur lors de la reception du pseudo");
                    close(client_socket);
                    continue;
                }

                pthread_mutex_lock(&mutex);
                if (is_pseudo_taken(pseudo)) {
                    char error_message[] = "Pseudo déjà utilisé. Connexion refusée.";
                    send(client_socket, error_message, sizeof(error_message), 0);
                    close(client_socket);
                    pthread_mutex_unlock(&mutex);
                    continue;
                } else {
                    strcpy(tab_client[i].pseudo, pseudo);
                    tab_client[i].socket = client_socket;
                    pthread_mutex_unlock(&mutex);
                }

                printf("%s s'est connecté\n", tab_client[i].pseudo);

                if (pthread_create(&tab_thread[i], NULL, broadcast, (void *)&tab_client[i]) != 0) {
                    perror("Erreur lors de la création du thread");
                    close(client_socket);
                    pthread_mutex_lock(&mutex);
                    tab_client[i].socket = -1;
                    pthread_mutex_unlock(&mutex);
                    continue;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENT; i++) {
            if (tab_client[i].socket != -1) {
                if (pthread_join(tab_thread[i], NULL) != 0) {
                    perror("Erreur lors de l'attente de la fin du thread");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }

    pthread_exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Error: You must provide exactly 1 argument.\nUsage: ./serv <port>\n");
        exit(EXIT_FAILURE);
    }

    printf("Debut du Serveur.\n");

    int dS = socket(PF_INET, SOCK_STREAM, 0);
    if (dS == -1) {
        perror("Erreur lors de la création du socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in ad;
    ad.sin_family = AF_INET;
    ad.sin_addr.s_addr = INADDR_ANY;
    ad.sin_port = htons(atoi(argv[1]));

    if (bind(dS, (struct sockaddr *)&ad, sizeof(ad)) == -1) {
        perror("Erreur lors du nommage du socket");
        exit(EXIT_FAILURE);
    }

    if (listen(dS, 10) == -1) {
        perror("Erreur lors du passage en mode écoute");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < MAX_CLIENT; i++) {
        tab_client[i].socket = -1;
    }

    pthread_mutex_init(&mutex, NULL);

    pthread_t relai;
    if (pthread_create(&relai, NULL, connexion, (void *)&dS) != 0) {
        perror("Erreur lors de la création du thread");
        exit(EXIT_FAILURE);
    }

    printf("En attente de connexion... \n");

    if (pthread_join(relai, NULL) != 0) {
        perror("Erreur lors de l'attente de la fin du thread");
        exit(EXIT_FAILURE);
    }

    pthread_mutex_destroy(&mutex);
    printf("Les clients sont déconnectés\n");

    if (close(dS) == -1) {
        perror("Erreur lors de la fermeture du socket de acceptation");
        exit(EXIT_FAILURE);
    }

    return 0;
}
