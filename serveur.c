#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#define MAX_CLIENT 2
#define NB_THREADS 2
#define BUFFER_SIZE 50

int tab_client[MAX_CLIENT];

// Fonction exécutée par chaque thread pour relayer les messages
void * broadcast(void * dS_client) {
    int dSC = *(int *)dS_client;
    char buffer[BUFFER_SIZE];
    int nb_recv;
    int nb_send;
    int i;

    // Boucle infinie pour écouter les messages du client
    while (1) {
        // Recevoir le message du client
        nb_recv = recv(dSC, buffer, BUFFER_SIZE, 0);
        if (nb_recv == -1) {
            perror("Erreur lors de la reception");
            exit(EXIT_FAILURE);
        }
        if (nb_recv == 0) {
            printf("Client déconnecté\n");
            break;
        }

        printf("Message recu: %s du client dSC: %d \n", buffer, dSC);

        // Si le client envoie "fin", arrêter la discussion pour ce client
        if (strcmp(buffer, "fin") == 0) {
            printf("Fin de la discussion pour client dSC: %d\n", dSC);
            break;
        }

        // Envoyer le message aux autres clients
        i = 0;
        while( i < MAX_CLIENT && tab_client[i] != 0){
            if (tab_client[i] != dSC) {
                nb_send = send(tab_client[i], buffer, BUFFER_SIZE, 0);
                if (nb_send == -1) {
                    perror("Erreur lors de l'envoi");
                    exit(EXIT_FAILURE);
                }
                if (nb_send == 0) {
                    printf("Le client dSC: %d s'est deconnecte\n", tab_client[i]);
                }
            }
            i = i + 1;
        }
    }

    // Fermer le socket du client
    if (close(dSC) == -1) {
        perror("Erreur lors de la fermeture du descripteur de fichier");
        exit(EXIT_FAILURE);
    }

    // Mettre 0 dans le tableau pour indiquer que le client s'est déconnecté
    *(int *)dS_client = 0;

    pthread_exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Error: You must provide exactly 1 argument.\nUsage: ./serv <port>\n");
        exit(EXIT_FAILURE);
    }  

    printf("Debut du Serveur.\n");

    int dS = socket(PF_INET, SOCK_STREAM, 0);
    if(dS == -1) {
        perror("Erreur lors de la création du socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in ad;
    ad.sin_family = AF_INET;
    ad.sin_addr.s_addr = INADDR_ANY ;
    ad.sin_port = htons(atoi(argv[1])) ;
    if(bind(dS, (struct sockaddr*)&ad, sizeof(ad)) == -1) {
        perror("Erreur lors du nommage du socket");
        exit(EXIT_FAILURE);
    }

    if(listen(dS, 10) == -1) {
        perror("Erreur lors du passage en mode écoute");
        exit(EXIT_FAILURE);
    }

    memset(tab_client, 0, sizeof(tab_client));
    struct sockaddr_in tab_adr[MAX_CLIENT];
    socklen_t tab_lg[MAX_CLIENT];

    printf("En attente de connexion des clients\n");

    // Accepter les connexions des clients
    for (int i = 0; i < MAX_CLIENT; i++){
        tab_lg[i] = sizeof(struct sockaddr_in);
        tab_client[i] = accept(dS, (struct sockaddr*) &tab_adr[i],&tab_lg[i]) ;
        if(tab_client[i] == -1) {
            perror("Erreur lors de la connexion avec le client");
            exit(EXIT_FAILURE);
        }
        printf("Client %d connecté\n", i+1);
    }
  
    pthread_t tid;
    pthread_t Threads_id [NB_THREADS] ;

    // Créer des threads pour gérer la communication avec les clients
    for (int i = 0; i < NB_THREADS; i++){
        if (pthread_create(&tid, NULL, broadcast, (void *) &tab_client[i]) != 0) {
            perror("Erreur lors de la création du thread");
            exit(EXIT_FAILURE);
        }
        else{
            Threads_id[i] = tid;
        }
    }

    // Attendre que les threads se terminent
    for (int i = 0; i < NB_THREADS; i++){
        if (pthread_join(Threads_id[i], NULL) != 0) {
            perror("Erreur lors de l'attente de la fin du thread");
            exit(EXIT_FAILURE);
        }
    }

    printf("Both clients disconnected\n");

    if (close(dS)==-1){
        perror("Erreur lors de la fermeture du socket de acceptation");
        exit(EXIT_FAILURE);
    }

    return 1;  
}
