#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <dirent.h>

#define MAX_LEN 1000
char msg[MAX_LEN];
char input[MAX_LEN];
char pseudo[MAX_LEN];

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int is_connected = 1;

void listFiles() {
    DIR *d;
    struct dirent *dir;
    d = opendir(".");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            printf("%s\n", dir->d_name);
        }
        closedir(d);
    }
}

void sendFileInfo(int dS, char *filename) {
    FILE *file = fopen(filename, "r");
    
    if (file == NULL) {
        perror("Erreur lors de l'ouverture du fichier");
        return;
    }

    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    rewind(file);
    
    char buffer[MAX_LEN];
    sprintf(buffer, "%s %ld", filename, filesize);
    printf("filename %s\n",buffer);
    send(dS, buffer, strlen(buffer), 0); // Send only the length of the buffer used
    //send(dS, buffer, strlen(buffer), 0);
    // Add a small delay to ensure the server processes this data before file data is sent
    usleep(1000);
}

void sendFile(int dS, char *filename) {
    FILE *file = fopen(filename, "r");
    
    if (file == NULL) {
        perror("Erreur lors de l'ouverture du fichier");
        return;
    }

    char buffer[MAX_LEN];
    size_t bytes;
    while ((bytes = fread(buffer, 1, MAX_LEN, file)) > 0) {
        send(dS, buffer, bytes, 0);
    }

    fclose(file);
}

void displayManual() {
    FILE *manuel = fopen("manuel.txt", "r");
    if (manuel == NULL) {
        perror("Erreur lors de l'ouverture du manuel");
        exit(EXIT_FAILURE);
    }

    char buffer[MAX_LEN];
    while (fgets(buffer, MAX_LEN, manuel) != NULL) {
        printf("%s", buffer);
    }

    fclose(manuel);
}

void *receiveFile(void *client)
{
    int dS = *(int *)client;
    char *file_name; // Allouer de la mémoire pour stocker le nom du fichier
    int nb_recv;
    long file_size;
    int bytes_received;
    FILE *file;
    char info[MAX_LEN];
    char content[MAX_LEN];

    // Recevoir le nom du fichier et la taille du fichier du serveur
    recv(dS, info, sizeof(info) - 1, 0);
    info[MAX_LEN - 1] = '\0';

    // Extraire le nom du fichier et la taille du fichier du tampon reçu
    sscanf(info, "%s %ld", file_name, &file_size);
    printf("%s %ld", file_name, file_size);

    // Créer un répertoire s'il n'existe pas déjà
    system("mkdir -p recu");

    // Ouvrir un fichier pour l'écriture
    char file_path[MAX_LEN];
    snprintf(file_path, sizeof(file_path), "recu/%s", file_name);
    file = fopen(file_path, "wb");
    if (file == NULL)
    {
        perror("Failed to open file");
        pthread_exit(NULL); // Sortir du thread en cas d'échec
    }

    // Recevoir le contenu du fichier
    int remaining_bytes = file_size;
    while (remaining_bytes > 0 && (nb_recv = recv(dS, content, sizeof(content), 0)) > 0)
    {
        fwrite(content, sizeof(char), nb_recv, file);
        remaining_bytes -= nb_recv;
    }
    fclose(file);

    printf("File transfer complete\n");

    pthread_exit(NULL);
}

void *readMessage(void *arg) {
    int dS = *(int *)arg;
    int nb_recv;
    while (1) {
        nb_recv = recv(dS, msg, MAX_LEN, 0);
        printf("msg: %s\n", msg);
        pthread_mutex_lock(&mutex);
        if (!is_connected) {
            break;
        } else if (strcmp(msg, "/fin") == 0) {
            printf("Un client a quitté la discussion\n");
        } else if (strncmp(msg, "/mp", 3) == 0) {
            char destinataire[MAX_LEN];
            char contenu[MAX_LEN];
            sscanf(msg, "/mp %s %[^\n]", destinataire, contenu);
            printf("Message privé envoyé à %s: %s\n", destinataire, contenu);
        } else if (strcmp(msg, "/man") == 0) {
            printf("Manuel :\n");
            displayManual();
        }
        else if (strcmp(msg, "filelist") == 0) {
            nb_recv = recv(dS, msg, MAX_LEN, 0);
            printf("Fichiers récupérables :\n%s\n", msg);
        }
        else if (strcmp(msg, "file") == 0){
            printf("testcdfdvdvfvc\n");
            pthread_t recevoirFichier;
            if (pthread_create(&recevoirFichier, NULL, receiveFile, (void *)&dS) != 0)
            {
                perror("Erreur lors de la création du thread");
                continue;
            }
        } else{
            printf("Message reçu: %s\n", msg);
        }
        pthread_mutex_unlock(&mutex);

        if (nb_recv == -1) {
            perror("Erreur lors de la reception du message");
            pthread_mutex_lock(&mutex);
            is_connected = 0;
            pthread_mutex_unlock(&mutex);
            exit(EXIT_FAILURE);
        } else if (nb_recv == 0) {
            printf("Le serveur a fermé la connexion\n");
            pthread_mutex_lock(&mutex);
            is_connected = 0;
            pthread_mutex_unlock(&mutex);
            break;
        }

        if (strcmp(msg, "Le serveur est down.") == 0) {
            pthread_mutex_lock(&mutex);
            is_connected = 0;
            pthread_mutex_unlock(&mutex);
            printf("Un client a interompu la communication\n");
            break;
        }
    }
    close(dS);
    pthread_exit(0);
}

void *writeMessage(void *arg) {
    int dS = *(int *)arg;
    int nb_send;

    nb_send = send(dS, pseudo, MAX_LEN, 0);
    if (nb_send == -1) {
        perror("Erreur lors de l'envoi du pseudo");
        pthread_mutex_lock(&mutex);
        is_connected = 0;
        pthread_mutex_unlock(&mutex);
        pthread_exit(NULL);
    }

    while (1) {
        fgets(input, MAX_LEN, stdin);
        char *pos = strchr(input, '\n');
        *pos = '\0';

        pthread_mutex_lock(&mutex);
        if (!is_connected) {
            break;
        }
        pthread_mutex_unlock(&mutex);

        if (strcmp(input, "/man") == 0) {
            printf("Manuel : \n");
            displayManual();
            printf("\n");
            continue;
        }
        else if (strncmp(input, "/file", 5) == 0)
        {
            listFiles();
            printf("Entrez le nom du fichier à envoyer : ");
            fgets(msg, MAX_LEN, stdin);
            msg[strcspn(msg, "\n")] = 0;
            nb_send = send(dS, input, MAX_LEN, 0);
            sendFileInfo(dS, msg);
            sendFile(dS, msg);
    }
    else if (strncmp(input, "/retrieve",9) == 0) {
        printf("Entrez le nom du fichier à récupérer : ");
        fgets(msg, MAX_LEN, stdin);
        msg[strcspn(msg, "\n")] = 0;
        nb_send = send(dS, input, MAX_LEN, 0);
        nb_send = send(dS, msg, MAX_LEN, 0);
       
    } else {

        printf("Message envoyé: %s\n", input);
    }


        nb_send = send(dS, input, MAX_LEN, 0);
        if (nb_send == -1) {
            perror("Erreur lors de l'envoi du message");
            pthread_mutex_lock(&mutex);
            is_connected = 0;
            pthread_mutex_unlock(&mutex);
            exit(EXIT_FAILURE);
        } else if (nb_send == 0) {
            printf("Le serveur a fermé la connexion\n");
            pthread_mutex_lock(&mutex);
            is_connected = 0;
            pthread_mutex_unlock(&mutex);
            break;
        }

        if (strcmp(input, "/fin") == 0 || strcmp(input, "/kill") == 0) {
            printf("Vous avez mis fin à la discussion\n");
            pthread_mutex_lock(&mutex);
            is_connected = 0;
            pthread_mutex_unlock(&mutex);
            break;
        }
    }

    pthread_exit(0);
}

void handle_sigint(int sig) {
    printf("\nPour quitter, veuillez saisir le mot 'fin'.\n");
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Error: You must provide exactly 3 arguments.\nUsage: %s <server_ip> <server_port> <pseudo>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("Début programme client\n");
    printf("Bienvenue sur la messagerie instantanée !\n");

    strcpy(pseudo, argv[3]);

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

    aS.sin_port = htons(atoi(argv[2]));
    socklen_t lgA = sizeof(struct sockaddr_in);
    if (connect(dS, (struct sockaddr *)&aS, lgA) == -1) {
        perror("Erreur connect client");
        exit(EXIT_FAILURE);
    }

    printf("Socket Connecté\n\n");

    signal(SIGINT, handle_sigint);

    pthread_t readThread, writeThread;
    pthread_mutex_init(&mutex, NULL);

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

    close(dS);
    pthread_mutex_destroy(&mutex);

    return EXIT_SUCCESS;
}
