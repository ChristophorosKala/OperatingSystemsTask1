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

typedef enum {
    STATE_IDLE,
    STATE_WAITING_VERIFICATION,
    STATE_WAITING_FINAL_ACK
} client_state_t;

/* Μετατροπή IPv4 από dotted string σε network-byte-order bytes */
static int parse_ipv4_address(const char *ip, struct in_addr *addr) {
    unsigned int octet1, octet2, octet3, octet4;
    char extra;

    if(sscanf(ip, "%u.%u.%u.%u%c", &octet1, &octet2, &octet3, &octet4, &extra) != 4) {
        return -1;
    }

    if(octet1 > 255 || octet2 > 255 || octet3 > 255 || octet4 > 255) {
        return -1;
    }

    addr->s_addr = htonl((octet1 << 24) | (octet2 << 16) | (octet3 << 8) | octet4);
    return 0;
}

/* Parse και εκτύπωση μόνο για απαντήσεις τύπου get */
static int print_get_response(const char *response) {
    int event_code;
    int brightness;
    int temperature_raw;
    long timestamp;

    if(sscanf(response, "%d %d %d %ld", &event_code, &brightness, &temperature_raw, &timestamp) != 4) {
        return 0;
    }

    const char *event_name = "unknown";

    switch(event_code) {
        case 0: event_name = "boot"; break;
        case 1: event_name = "setup"; break;
        case 2: event_name = "interval"; break;
        case 3: event_name = "button"; break;
        case 4: event_name = "motion"; break;
    }

    double temperature = temperature_raw / 100.0;
    time_t ts = (time_t)timestamp;
    struct tm *tm_info = localtime(&ts);

    if(tm_info != NULL) {
        char time_buffer[64];

        if(strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", tm_info) > 0) {
            /* Exact exercise output format */
            printf("Latest event:\n");
            printf("%s (%d)\n", event_name, event_code);
            printf("Temperature is: %.2f\n", temperature);
            printf("Light level is: %d\n", brightness);
            printf("Timestamp is: %s\n", time_buffer);
            return 1;
        }
    }

    /* Fallback: print numeric timestamp if time formatting failed */
    printf("Latest event:\n");
    printf("%s (%d)\n", event_name, event_code);
    printf("Temperature is: %.2f\n", temperature);
    printf("Light level is: %d\n", brightness);
    printf("Timestamp is: %ld\n", timestamp);

    return 1;
}

static int read_server_message(int sockfd, char *buffer, size_t buffer_size) {
    int n = read(sockfd, buffer, buffer_size - 1);

    if(n <= 0) {
        return n;
    }

    buffer[n] = '\0';
    buffer[strcspn(buffer, "\n")] = '\0';
    return n;
}

static int send_all(int sockfd, const char *buffer, size_t length) {
    size_t total_sent = 0;

    while(total_sent < length) {
        ssize_t written = write(sockfd, buffer + total_sent, length - total_sent);

        if(written < 0) {
            return -1;
        }

        total_sent += (size_t)written;
    }

    return 0;
}

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
    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    /* Χειροκίνητη μετατροπή IP σε binary μορφή χωρίς DNS/name resolution */
    if(parse_ipv4_address(ip, &server_addr.sin_addr) < 0) {
        fprintf(stderr, "Invalid IPv4 address: %s\n", ip);
        close(sockfd);
        return 1;
    }

    /* Σύνδεση με server */
    if(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
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
    client_state_t client_state = STATE_IDLE;

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
                close(sockfd); // Κλείσιμο socket πριν την έξοδο
                break;
            }

            /* Εμφάνιση help */
            if(strcmp(send_buffer, "help") == 0) {
                print_help();
                continue;
            }
            
            /* Αποστολή δεδομένων στον server */
            if(send_all(sockfd, send_buffer, strlen(send_buffer)) < 0) {
                perror("write");
                break;
            }

            /* Ειδικός χειρισμός για αίτημα άδειας */
            if(send_buffer[0] >= '0' && send_buffer[0] <= '9' && strchr(send_buffer, ' ') != NULL) {
                client_state = STATE_WAITING_VERIFICATION;
                continue;
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
            int n = read_server_message(sockfd, recv_buffer, sizeof(recv_buffer));

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
            
            /* Debug πληροφορίες */
            if(debug) {
                printf("[DEBUG] read '%s'\n", recv_buffer);
            }

            /* Χειρισμός verification handshake χωρίς να μπλοκάρει το poll() */
            if(client_state == STATE_WAITING_VERIFICATION) {
                if(strcmp(recv_buffer, "try again") == 0) {
                    printf("Server: %s\n", recv_buffer);
                    client_state = STATE_IDLE;
                    continue;
                }

                printf("Send verification code: %s\n", recv_buffer);

                if(send_all(sockfd, recv_buffer, strlen(recv_buffer)) < 0) {
                    perror("write");
                    break;
                }

                client_state = STATE_WAITING_FINAL_ACK;
                continue;
            }

            if(client_state == STATE_WAITING_FINAL_ACK) {
                printf("Server: %s\n", recv_buffer);
                client_state = STATE_IDLE;
                continue;
            }

            /* Αν είναι απάντηση get, την αποκωδικοποιούμε ξεχωριστά */
            if(!print_get_response(recv_buffer)) {
                printf("Server: %s\n", recv_buffer);
            }
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
