#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <curl/curl.h>

#define SERVER_IP "QUA DEVI INSERIRE L'IP PUBBLICO"
#define SERVER_PORT 8888
#define BUFFER_SIZE 1024

int client_socket;
pthread_t receive_thread;

void *receive_messages(void *arg) {
    char buffer[BUFFER_SIZE];
    ssize_t num_bytes;

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        num_bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (num_bytes <= 0) {
            break;
        } else {
            printf("%s\n", buffer);
        }
    }

    printf("Connessione al server terminata.\n");
    pthread_exit(NULL);
    return 0;
}

      
void check_ip() {

    CURL *curl;
    CURLcode res;
    char *url = "https://api.ipify.org/?format=json"; // Un servizio per ottenere l'IP pubblico

    // Inizializza libcurl
    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Errore nell'inizializzazione di libcurl\n");
    }

    // Configura l'URL di destinazione
    curl_easy_setopt(curl, CURLOPT_URL, url);

    // Esegui la richiesta HTTP
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "Errore nella richiesta HTTP: %s\n", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
    }

    // Chiudi la connessione HTTP
    curl_easy_cleanup(curl);
}

int main() {
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char client_name[BUFFER_SIZE];

    check_ip();

    // Creazione del socket del client
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Errore nella creazione del socket");
        exit(EXIT_FAILURE);
    }

    // Impostazione dell'indirizzo del server
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_IP, &(server_addr.sin_addr)) <= 0) {
        perror("Errore nell'indirizzo del server");
        exit(EXIT_FAILURE);
    }

    // Connessione al server
    if (connect(client_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("Errore nella connessione al server");
        exit(EXIT_FAILURE);
    }

    printf("\nConnessione al server %s:%d avvenuta con successo.\n", SERVER_IP, SERVER_PORT);

    // Inserimento del nome del client
    printf("Inserisci il tuo nome: ");
    fgets(client_name, sizeof(client_name), stdin);
    client_name[strcspn(client_name, "\n")] = '\0';

    // Invio del nome del client al server
    if (send(client_socket, client_name, strlen(client_name), 0) < 0) {
        perror("Errore nell'invio del nome del client");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    // Creazione del thread per la ricezione dei messaggi
    if (pthread_create(&receive_thread, NULL, receive_messages, NULL) != 0) {
        perror("Errore nella creazione del thread");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    // Ciclo di input dei messaggi
    while (1) {
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = '\0';

        if (strcmp(buffer, "exit") == 0) {
            printf("Disconnessione dal server.\n");

            if (send(client_socket, "Il client ha abbandonato", 30, 0) < 0) {
            perror("Errore nell'invio del messaggio");
            break; //non un'implementazione raffinata ma funziona
        }
            return 0;
        }

        // Invio del messaggio al server
        if (send(client_socket, buffer, strlen(buffer), 0) < 0) {
            perror("Errore nell'invio del messaggio");
            break;
        }

    }

    // Join del thread di ricezione dei messaggi
    pthread_join(receive_thread, NULL);

    // Chiusura del socket del client
    close(client_socket);

    return 0;
}