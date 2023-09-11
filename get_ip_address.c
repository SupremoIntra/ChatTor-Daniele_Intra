#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

// Funzione di callback per scrivere la risposta HTTP in una stringa
size_t write_callback(void *contents, size_t size, size_t nmemb, char **output) {
    size_t total_size = size * nmemb;
    *output = (char *)malloc(total_size + 1); // +1 per il terminatore null
    if (*output) {
        memcpy(*output, contents, total_size);
        (*output)[total_size] = '\0'; // Aggiungi il terminatore null alla fine
    }
    return total_size;
}

int main() {
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if (curl) {
        // Imposta l'URL del servizio di ottenimento dell'indirizzo IP
        const char *url = "https://ifconfig.me/ip";

        // Variabile per contenere la risposta HTTP
        char *response = NULL;

        // Imposta le opzioni cURL
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        // Esegui la richiesta HTTP
        res = curl_easy_perform(curl);

        // Verifica se la richiesta ha avuto successo
        if (res != CURLE_OK) {
            fprintf(stderr, "Errore nella richiesta cURL: %s\n", curl_easy_strerror(res));
        } else {
            // Stampa l'indirizzo IP pubblico ottenuto dalla risposta
            printf("Il tuo indirizzo IP pubblico è: %s\n", response);
        }

        // Rilascia le risorse
        free(response);
        curl_easy_cleanup(curl);
    }

    return 0;
}
