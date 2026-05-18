#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <poll.h>
#include <time.h>

#define DEFAULT_IP "147.102.75.201"
#define DEFAULT_PORT 4217
#define BUFFER_SIZE 1024

/* Εκτύπωση διαθέσιμων εντολών */
void print_help() {
    
    printf("Available commands:\n");
    printf("help : Print help message\n");
    printf("exit : Exit program\n");
    printf("get : Get latest sensor data\n");
    printf("N name surname reason : Request movement permission\n");
}

int main(int argc, char *argv[]) {

    /* Προεπιλεγμένες τιμές IP και port */
    char ip[64] = DEFAULT_IP;
    int port = DEFAULT_PORT;

    /* Flag για debug mode */
    int debug = 0;

    /* Διαβάζουμε arguments από terminal */
    for(int i = 1; i < argc; i++) {

        /* Αλλαγή IP */
        if(strcmp(argv[i], "--ip") == 0 && i + 1 < argc) {
            strcpy(ip, argv[++i]);
        }

        /* Αλλαγή port */
        else if(strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = atoi(argv[++i]);
        }

        /* Ενεργοποίηση debug */
        else if(strcmp(argv[i], "--debug") == 0) {
            debug = 1;
        }
    }

    /* Δημιουργία socket */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if(sockfd < 0) {
        perror("socket");
        return 1;
    }

    /* Δομή διεύθυνσης server */
    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    /* Μετατροπή IP σε binary μορφή */
    if(inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        return 1;
    }

    /* Σύνδεση με server */
    if(connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sockfd);
        return 1;
    }

    printf("Connected to %s:%d\n", ip, port);

    /* Εκτύπωση βοήθειας */
    print_help();

    /* poll descriptors:
       fds[0] : terminal
       fds[1] :  socket
    */
    struct pollfd fds[2];

    /* Παρακολούθηση stdin */
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;

    /* Παρακολούθηση socket */
    fds[1].fd = sockfd;
    fds[1].events = POLLIN;

    /* Buffers αποστολής/λήψης */
    char send_buffer[BUFFER_SIZE];
    char recv_buffer[BUFFER_SIZE];

    /* Κεντρικό loop client */
    while(1) {

        /* Περιμένουμε activity σε terminal ή socket */
        int ready = poll(fds, 2, -1);

        if(ready < 0) {
            perror("poll");
            break;
        }
        
        /* =========================================
           Δεδομένα από terminal
           ========================================= */
        if(fds[0].revents & POLLIN) {

            /* Καθαρισμός buffer */
            memset(send_buffer, 0, BUFFER_SIZE);

            /* Διάβασμα γραμμής από χρήστη */
            if(fgets(send_buffer, BUFFER_SIZE, stdin) == NULL) {
                break;
            }

            /* Αφαίρεση newline */
            send_buffer[strcspn(send_buffer, "\n")] = '\0';

            /* Έξοδος προγράμματος */
            if(strcmp(send_buffer, "exit") == 0) {
                printf("Exiting...\n");
                break;
            }

            /* Εμφάνιση help */
            if(strcmp(send_buffer, "help") == 0) {
                print_help();
                continue;
            }
            
            /* Αποστολή δεδομένων στον server */
            if(write(sockfd, send_buffer, strlen(send_buffer)) < 0) {
                perror("write");
                break;
            }
            
            /* Debug πληροφορίες */
            if(debug) {
                printf("[DEBUG] sent '%s'\n", send_buffer);
            }
        }    
        
        /* =========================================
           Δεδομένα από server
           ========================================= */
        if(fds[1].revents & POLLIN) {
            
            /* Καθαρισμός receive buffer */
            memset(recv_buffer, 0, BUFFER_SIZE);

            /* Διαβάζουμε δεδομένα από socket */
            int n = read(sockfd, recv_buffer, BUFFER_SIZE - 1);
            
            /* Έλεγχος σφάλματος */
            if(n == -1) {
                perror("read");
                break;
            }
            
            /* Ο server έκλεισε τη σύνδεση */
            if(n == 0) {
                printf("Server disconnected\n");
                break;
            }
            
            /* Προσθήκη '\0' στο τέλος */
            recv_buffer[n] = '\0';

            /* Αφαίρεση newline */
            recv_buffer[strcspn(recv_buffer, "\n")] = '\0';
            
            /* Debug πληροφορίες */
            if(debug) {
                printf("[DEBUG] read '%s'\n", recv_buffer);
            }

            /* Εκτύπωση απάντησης server */
            printf("Server: %s\n", recv_buffer);
        }
        
        /* =========================================
           Εδώ πρέπει να βάλουμε επιπλέον έλεγχο
           για απαντήσεις τύπου "get"
           ========================================= */
    }

    /* Κλείσιμο socket */
    close(sockfd);

    return 0;
}
