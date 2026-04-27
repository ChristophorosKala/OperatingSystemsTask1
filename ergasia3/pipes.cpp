#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <sys/select.h>
#include <errno.h>

/ Μια ειδική global μεταβλητή (ασφαλής για σήματα)
volatile sig_atomic_t exit_requested = 0;

#define STRAT_ROUND_ROBIN 0
#define STRAT_RANDOM 1

// Forced execution break handler flag
void handle_sigint(int sig) {
    exit_requested = 1;
}

// ==========================================
//            ΚΩΔΙΚΑΣ ΠΑΙΔΙΟΥ
// ==========================================
void child_logic(int child_index, int read_fd, int write_fd) {
    int val;
    // Το παιδί περιμένει δουλειά από τον πατέρα
    while (read(read_fd, &val, sizeof(int)) > 0) {
        printf("[Child %d] [%d] Child received %d!\n", child_index, getpid(), val);
        val *= 2;
        sleep(5); // Προσομοίωση βαριάς εργασίας
        
        // ERROR HANDLING στην εγγραφή
        if (write(write_fd, &val, sizeof(int)) == -1) {
            perror("Child failed to write back");
        } else {
            printf("[Child %d] [%d] Child Finished hard work, writing back %d\n", child_index, getpid(), val);
        }
    }
    // Αν ο πατέρας κλείσει το pipe, η read επιστρέφει 0 και το παιδί τερματίζει ομαλά
    exit(0); 
}

// ==========================================
//            ΚΩΔΙΚΑΣ ΠΑΤΕΡΑ
// ==========================================
void parent_logic(int n, int strategy, int (*p2c)[2], int (*c2p)[2], pid_t *pids) {
    char buffer[256];
    int rr_index = 0;
    fd_set readfds;

    printf("Parent process started. Waiting for commands...\n");

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds); // Ακούει το πληκτρολόγιο
        int max_fd = STDIN_FILENO;

        // Ακούει όλα τα παιδιά
        for (int i = 0; i < n; i++) {
            FD_SET(c2p[i][0], &readfds);
            if (c2p[i][0] > max_fd) max_fd = c2p[i][0];
        }

        // ERROR HANDLING στη select
        if (select(max_fd + 1, &readfds, NULL, NULL, NULL) == -1) {
            // Αν διακόπηκε από το Ctrl+C (SIGINT)
            if (errno == EINTR && exit_requested == 1) {
                printf("\n[Ctrl+C detected] Exiting gracefully...\n");
                // Διαδικασία Τερματισμού (Ακριβώς όπως η "exit")
                for (int i = 0; i < n; i++) {
                    kill(pids[i], SIGTERM); 
                    waitpid(pids[i], NULL, 0); 
                }
                printf("All children terminated\n");
                return; // Επιστρέφει στη main για το free()
            } else {
                perror("Select failed");
                break;
            }
        }
	
        // -- 1. Ήρθε εντολή από τον χρήστη (Terminal) --
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            
            // EDGE CASE: EOF (Ctrl+D)
            if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
                printf("\nEOF detected. Exiting gracefully...\n");
                for (int i = 0; i < n; i++) {
                    kill(pids[i], SIGTERM); 
                    waitpid(pids[i], NULL, 0); 
                }
                printf("All children terminated\n");
                return;
            }

            buffer[strcspn(buffer, "\n")] = 0; // Αφαίρεση του \n

            if (strcmp(buffer, "help") == 0) {
                printf("Type a number to send job to a child!\n");
            } 
            else if (strcmp(buffer, "exit") == 0) {
                // Διαδικασία Τερματισμού
                for (int i = 0; i < n; i++) {
                    kill(pids[i], SIGTERM); 
                    waitpid(pids[i], NULL, 0); 
                }
                printf("All children terminated\n");
                return; 
            } 
            else {
                char *endptr;
                int job_val = strtol(buffer, &endptr, 10);

                if (endptr != buffer && *endptr == '\0') { // Αν είναι έγκυρος αριθμός
                    int target_child = (strategy == STRAT_ROUND_ROBIN) ? rr_index : rand() % n;
                    if (strategy == STRAT_ROUND_ROBIN) rr_index = (rr_index + 1) % n;

                    // ERROR HANDLING στην εγγραφή προς το παιδί
                    if (write(p2c[target_child][1], &job_val, sizeof(int)) == -1) {
                        perror("Failed to send job to child");
                    } else {
                        printf("[Parent] Assigned %d to child %d\n", job_val, target_child);
                    }
                } else {
                    printf("Type a number to send job to a child!\n");
                }
            }
        }

        // -- 2. Ήρθε απάντηση από κάποιο παιδί --
        for (int i = 0; i < n; i++) {
            if (FD_ISSET(c2p[i][0], &readfds)) {
                int result;
                // ERROR HANDLING στην ανάγνωση
                if (read(c2p[i][0], &result, sizeof(int)) == -1) {
                    perror("Failed to read from child");
                }
            }
        }
    }
}

// ==========================================
//        ΚΕΝΤΡΙΚΟΣ ΣΥΝΤΟΝΙΣΤΗΣ (main)
// ==========================================
int main(int argc, char *argv[]) {
    // 1. Έλεγχος Ορισμάτων
    if (argc < 2 || argc > 3) {
        printf("Usage: %s <n> [--random] [--round-robin]\n", argv[0]);
        exit(1);
    }

    int n = atoi(argv[1]);
    if (n <= 0) {
        printf("Error: <n> must be a positive integer.\n");
        exit(1);
    }

    int strategy = STRAT_ROUND_ROBIN;
    if (argc == 3) {
        if (strcmp(argv[2], "--random") == 0) strategy = STRAT_RANDOM;
        else if (strcmp(argv[2], "--round-robin") == 0) strategy = STRAT_ROUND_ROBIN;
        else {
            printf("Usage: %s <n> [--random] [--round-robin]\n", argv[0]);
            exit(1);
        }
    }

    // EDGE CASE: Πες στο σύστημα να στέλνει το Ctrl+C στον Handler σου (αντί να σε σκοτώνει)
    // Πιάσε το Ctrl+C (SIGINT)
    signal(SIGINT, handle_sigint);

    // EDGE CASE: Αποφυγή θανάτου του πατέρα αν σπάσει κάποιο pipe
    signal(SIGPIPE, SIG_IGN);

    // 2. Προετοιμασία (Μνήμη & Pipes)
    srand(time(NULL));
    
    // Προσθήκη (cast) για συμβατότητα με C++ (.cpp files)
    int (*p2c)[2] = (int (*)[2])malloc(n * sizeof(int[2]));
    int (*c2p)[2] = (int (*)[2])malloc(n * sizeof(int[2]));
    pid_t *pids = (pid_t *)malloc(n * sizeof(pid_t));

    // ERROR HANDLING για τη μνήμη
    if (p2c == NULL || c2p == NULL || pids == NULL) {
        perror("Failed to allocate memory");
        exit(1);
    }

    // 3. Δημιουργία Παιδιών (Forking)
    for (int i = 0; i < n; i++) {
        // ERROR HANDLING για τα Pipes
        if (pipe(p2c[i]) == -1 || pipe(c2p[i]) == -1) {
            perror("Failed to create pipes");
            exit(1);
        }

        pid_t pid = fork();
        
        // ERROR HANDLING για την Fork
        if (pid == -1) {
            perror("Failed to fork process");
            exit(1);
        }

        if (pid == 0) {
            // Το παιδί κλείνει τα άκρα που δεν χρειάζεται
            for (int j = 0; j <= i; j++) {
                close(p2c[j][1]); 
                if (j < i) close(p2c[j][0]); 
                close(c2p[j][0]); 
                if (j < i) close(c2p[j][1]); 
            }
            child_logic(i, p2c[i][0], c2p[i][1]);
        } else {
            pids[i] = pid;
            close(p2c[i][0]); 
            close(c2p[i][1]); 
        }
    }

    // 4. Ο πατέρας ξεκινάει τη δική του λογική
    parent_logic(n, strategy, p2c, c2p, pids);

    // 5. Καθαρισμός Μνήμης μετά τον τερματισμό
    free(p2c);
    free(c2p);
    free(pids);
    
    return 0;
}
