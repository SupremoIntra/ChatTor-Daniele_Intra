#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

int client_sockets[MAX_CLIENTS];
pthread_t client_threads[MAX_CLIENTS];
pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;
char client_names[MAX_CLIENTS][BUFFER_SIZE];

void *handle_client(void *arg) {
    int client_socket = *(int *) arg;
    char buffer[BUFFER_SIZE];
    ssize_t num_bytes;

    // Ricezione del nome del client
    memset(buffer, 0, sizeof(buffer));
    num_bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (num_bytes <= 0) {
        close(client_socket);
        pthread_exit(NULL);
    }
    buffer[num_bytes] = '\0';
    strncpy(client_names[client_socket], buffer, sizeof(client_names[client_socket]) - 1);

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        num_bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (num_bytes <= 0) {
            break;
        } else {
            printf("Messaggio ricevuto dal client %s: %s\n", client_names[client_socket], buffer);

            pthread_mutex_lock(&client_mutex);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] != 0 && client_sockets[i] != client_socket) {
                    char message[4096];
                    snprintf(message, sizeof(message), "%s: %s", client_names[client_socket], buffer);
                    if (send(client_sockets[i], message, strlen(message), 0) < 0) {
                        perror("Errore nell'invio del messaggio al client");
                        break;
                    }
                }
            }
            pthread_mutex_unlock(&client_mutex);
        }
    }

    pthread_mutex_lock(&client_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] == client_socket) {
            client_sockets[i] = 0;
            break;
        }
    }
    pthread_mutex_unlock(&client_mutex);

    close(client_socket);
    pthread_exit(NULL);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;
    char buffer[BUFFER_SIZE];
    int num_clients = 0;

    // Creazione del socket del server
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Errore nella creazione del socket del server");
        exit(EXIT_FAILURE);
    }

    // Impostazione dell'indirizzo del server
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8888);

    // Binding del socket del server
    if (bind(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("Errore nel binding del socket del server");
        exit(EXIT_FAILURE);
    }

    // Inizio dell'ascolto delle connessioni in arrivo
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("Errore nell'ascolto delle connessioni in arrivo");
        exit(EXIT_FAILURE);
    }

    printf("Server avviato. In attesa di connessioni...\n");

    while (1) {
        // Accettazione di una nuova connessione
        client_addr_len = sizeof(client_addr);
        if ((client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &client_addr_len)) < 0) {
            perror("Errore nell'accettazione di una nuova connessione");
            exit(EXIT_FAILURE);
        }

        // Verifica se ci sono slot liberi per nuovi client
        if (num_clients >= MAX_CLIENTS) {
            printf("Numero massimo di client raggiunto. Nuova connessione rifiutata.\n");
            close(client_socket);
            continue;
        }

        pthread_mutex_lock(&client_mutex);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] == 0) {
                client_sockets[i] = client_socket;
                num_clients++;
                break;
            }
        }
        pthread_mutex_unlock(&client_mutex);

        // Creazione di un thread per gestire il client
        if (pthread_create(&client_threads[num_clients - 1], NULL, handle_client, (void *) &client_socket) != 0) {
            perror("Errore nella creazione del thread per il client");
            exit(EXIT_FAILURE);
        }

        printf("Nuova connessione accettata. Numero di client connessi: %d\n", num_clients);

        // Invio del messaggio di benvenuto al client
        snprintf(buffer, sizeof(buffer), "Benvenuto, client %d!", num_clients);
        if (send(client_socket, buffer, strlen(buffer), 0) < 0) {
            perror("Errore nell'invio del messaggio di benvenuto al client");
        }
    }

    return 0;
}