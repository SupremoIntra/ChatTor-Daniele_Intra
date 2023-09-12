#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>
#define _GNU_SOURCE

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

struct ClientInfo {
    int socket;
    char name[BUFFER_SIZE];
    struct ClientInfo* next;
};

int server_socket;
int num_clients = 0;
int flag = 0;
struct ClientInfo* clients = NULL;
pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;

void handle_shutdown(int sig);
void* handle_client(void* arg);

int main() {
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;
    pthread_t client_threads[MAX_CLIENTS];

    // Dichiarazione e inizializzazione del gestore del segnale
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_shutdown;

    // Configurazione del gestore per SIGINT (Ctrl+C)
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Errore nell'impostazione del gestore di segnali");
        exit(EXIT_FAILURE);
    }

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Errore nella creazione del socket del server");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8888);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Errore nel binding del socket del server");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("Errore nell'ascolto delle connessioni in arrivo");
        exit(EXIT_FAILURE);
    }

    printf("Server avviato. In attesa di connessioni...\n");

    while (!flag) {
        client_addr_len = sizeof(client_addr);
        int client_socket;
        if ((client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len)) < 0) {
            perror("Errore nell'accettazione di una nuova connessione");
            exit(EXIT_FAILURE);
        }

        if (num_clients >= MAX_CLIENTS) {
            printf("Numero massimo di client raggiunto. Nuova connessione rifiutata.\n");
            close(client_socket);
            continue;
        }

        pthread_mutex_lock(&client_mutex);
        if (clients == NULL) {
            clients = (struct ClientInfo*)malloc(sizeof(struct ClientInfo));
            clients->socket = client_socket;
            memset(clients->name, 0, sizeof(clients->name));
            clients->next = NULL;
        } else {
            struct ClientInfo* current = clients;
            while (current->next != NULL) {
                current = current->next;
            }
            current->next = (struct ClientInfo*)malloc(sizeof(struct ClientInfo));
            current->next->socket = client_socket;
            memset(current->next->name, 0, sizeof(current->next->name));
            current->next->next = NULL;
        }
        num_clients++;
        pthread_mutex_unlock(&client_mutex);

        printf("Nuovo client connesso. Numero di client connessi: %d\n", num_clients);

        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, handle_client, (void*)&client_socket) != 0) {
            perror("Errore nella creazione del thread per il client");
            exit(EXIT_FAILURE);
        }
    }

    close(server_socket);

    struct ClientInfo* current = clients;
    while (current != NULL) {
        struct ClientInfo* next = current->next;
        close(current->socket);
        free(current);
        current = next;
    }

    return 0;
}

void handle_shutdown(int sig) {
    flag = 1;
    printf("\nServer terminato. Impossibile accettare nuove connessioni.\n");
}

void* handle_client(void* arg) {
    int client_socket = *(int*)arg;
    char buffer[BUFFER_SIZE];
    ssize_t num_bytes;

    memset(buffer, 0, sizeof(buffer));
    num_bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (num_bytes <= 0) {
        close(client_socket);
        pthread_exit(NULL);
    }
    buffer[num_bytes] = '\0';

    pthread_mutex_lock(&client_mutex);
    struct ClientInfo* current = clients;
    while (current != NULL) {
        if (current->socket == client_socket) {
            strncpy(current->name, buffer, sizeof(current->name) - 1);
            break;
        }
        current = current->next;
    }
    pthread_mutex_unlock(&client_mutex);

    snprintf(buffer, sizeof(buffer), "Benvenuto, client %s!", current->name);
    if (send(client_socket, buffer, strlen(buffer), 0) < 0) {
        perror("Errore nell'invio del messaggio di benvenuto al client");
        pthread_exit(NULL);
    }

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        num_bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (num_bytes <= 0) {
            break;
        } else {
            printf("Messaggio ricevuto dal client %s: %s\n", current->name, buffer);

            pthread_mutex_lock(&client_mutex);
            struct ClientInfo* sender = clients;
            while (sender != NULL) {
                if (sender->socket != 0 && sender->socket != client_socket) {
                    char message[4096];
                    snprintf(message, sizeof(message), "%s: %s", current->name, buffer);
                    if (send(sender->socket, message, strlen(message), 0) < 0) {
                        perror("Errore nell'invio del messaggio al client");
                        break;
                    }
                }
                sender = sender->next;
            }
            pthread_mutex_unlock(&client_mutex);
        }
    }

    pthread_mutex_lock(&client_mutex);
    struct ClientInfo* prev = NULL;
    current = clients;
    while (current != NULL) {
        if (current->socket == client_socket) {
            if (prev != NULL) {
                prev->next = current->next;
            } else {
                clients = current->next;
            }
            free(current);
            break;
        }
        prev = current;
        current = current->next;
    }
    num_clients--;
    pthread_mutex_unlock(&client_mutex);

    close(client_socket);
    pthread_exit(NULL);
}