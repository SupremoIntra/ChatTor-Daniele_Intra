#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAXLEN 255
#define PORT 8080
#define LISTEN 5

void die(char *);
void handleClient(int);

int main () {
    int socketFD, csd;
	struct sockaddr_in bind_address, client_ip_port;
	int bind_address_length = sizeof(bind_address);
	int client_ip_port_length = sizeof(client_ip_port);

    socketFD = socket(AF_INET, SOCK_STREAM, 0); //creiamo la socket TCP
	
	if (socketFD < 0) die("socket() error");
	printf("socket creata con successo.\n");

    bind_address.sin_family = AF_INET;
	bind_address.sin_addr.s_addr = inet_addr("127.0.0.1"); //valorizziamo indirizzo IP e porta del server
	bind_address.sin_port = htons(PORT);

    if (bind(socketFD, (struct sockaddr *) &bind_address, bind_address_length) < 0) die("bind() error"); //facciamo il binding al server
	printf("bind() ok.\n");

    //listen del server
    if (listen(socketFD, LISTEN) < 0) die("listen() error");
    printf("listen avvenuta con successo.\n");
	
	while (1) {
		csd = accept(socketFD, NULL, NULL); //il doppio NULL mette a NULL la struttura dati dell'indirizzo
        //il ritorno della funzione accept non ci dirà l'indirizzo del client.
		handleClient(csd); //qui passiamo la struttura con i dati (IP:port) del client
	}

    close(socketFD);

	return 0;
} 

void handleClient(int sd) {
	
	char sendbuff[MAXLEN], recvbuff[MAXLEN];
	
	memset(sendbuff, 0, MAXLEN); //inizializzazione dei buffer
	memset(recvbuff, 0, MAXLEN);
	
	int byterecvd = recv(sd, recvbuff, MAXLEN, 0);
	if (byterecvd <= 0) die("recv() error");
	printf("%d byte ricevuti\n", byterecvd);
		
	strcpy(sendbuff, recvbuff); //copio quello che ho ricevuto dal client e lo rimando
	
	int bytesent = send(sd, sendbuff, MAXLEN, 0);
	if (bytesent <= 0) die("send() error");
	printf("%d byte inviati\n", bytesent);

    close(sd);
	
}

void die(char *error) {
	
	fprintf(stderr, "%s.\n", error);
	exit(1);
	
}