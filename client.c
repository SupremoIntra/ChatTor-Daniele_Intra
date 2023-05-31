#include <stdio.h>
//#include <winsock2.h>  libreria per windows
//#pragma comment(lib, "ws2_32.lib")
#include <sys/socket.h>  //libreria che implementa le socket
#include <netinet/in.h>


int main() {

    /*introduco la mia funzione socket
    essa prende come input:
    1. la famiglia di indirizzi che voglio usare (IPv4 -> Address Family INET)
    2. che tipo di socket vogliamo implementare: TCP(SOCK_STREAM) o UDP
    3. specifichiamo 0 (di solito di default) -> vogliamo il livello IP come 
       livello da usare sotto il nostro livello trasporto
    
    La funzione socket ritorna un intero, un socket file descriptor (uso una variabile)
    Se mi dovesse ritornare un num positivo --> creazione socket con successo
    Se mi dovesse ritornare un num negativo (-1) --> problema */
    int socketFD = socket(AF_INET, SOCK_STREAM, 0);

    /*ora possiamo usare il file descriptor che abbiamo creato per connetterci 
    ad un server remoto --> per fare questo usiamo la funzione connect()
    la funzione connect prende come input:
    1. il nostro file descriptor
    2. l'indirizzo + porta del server a cui vogliamo connetterci (passo l'indirizzo 
       della struct che contiene ip+porta)
    3. la lunghezza dell'indirizzo */
    
    struct sockaddr_in address;//struct che definisce l'indirizzo ipv4
    address.sin_family = AF_INET; //specifico ancora famiglia ipv4
    address.sin_port = htons(80); //numero della porta su cui sta ascoltando il server dall'altra parte
    /*htons --> host to network short: converto l'ordine di byte della macchina
    nell'ordine di byte usato di default nelle reti (Big endian)


    Devo anche specificare l'indirizzo vero e proprio: uso una funzione che mi 
    formatta l'ip in modo intuitivo --> Presentation TO Network*/
    char* ip = "142.250.188.46"; 
    inet_pton(AF_INET, ip, &address.sin_addr.s_addr);
    
    //anche la funzione connect ritorna un intero per capire se è andata a buon fine o meno
    int result = connect(socketFD, &address, sizeof address);

    if(result==0)
    printf("connessione riuscita :)\n");


    return 0;
}