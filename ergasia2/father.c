#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

int num_children;
pid_t *child_pids;

// Συνάρτηση εκτύπωσης δέντρου
void print_process_tree() {
    printf("\n--- ΟΠΤΙΚΟΠΟΙΗΣΗ ΔΕΝΤΡΟΥ ΔΙΕΡΓΑΣΙΩΝ ---\n");
    printf("Πατέρας (PID: %d)\n", getpid());
    
    for (int i = 0; i < num_children; i++) {
        // Αν είναι το τελευταίο παιδί, τυπώνει └── αλλιώς ├──
        if (i == num_children - 1) {
            printf(" └── Παιδί %d (PID: %d)\n", i, child_pids[i]);
        } else {
            printf(" ├── Παιδί %d (PID: %d)\n", i, child_pids[i]);
        }
    }
    printf("----------------------------------------\n\n");
}

// Συνάρτηση για τη δημιουργία ενός παιδιού σε συγκεκριμένη θέση του πίνακα
void spawn_child(int i) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        exit(1);
    }
    if (pid == 0) {
        // Είμαστε στο παιδί - Φόρτωση του child εκτελέσιμου
        char *args[] = {"./child", NULL};
        execv(args[0], args);
        // Αν φτάσει εδώ, η execv απέτυχε
        perror("Execv failed");
        exit(1);
    } else {
        // Είμαστε στον πατέρα - Αποθήκευση του PID
        child_pids[i] = pid;
        printf("[Father] Created child %d with PID: %d\n", i, pid);
    }
}

// Handler για την προώθηση σημάτων στα παιδιά
void forward_signal(int sig) {
    printf("[Father] Forwarding signal %d to all children...\n", sig);
    for (int i = 0; i < num_children; i++) {
        if (child_pids[i] > 0) {
            kill(child_pids[i], sig);
        }
    }
    // Αν ο πατέρας λάβει SIGTERM, πρέπει να κλείσει και ο ίδιος αφού ενημερώσει τα παιδιά
    if (sig == SIGTERM) {
        free(child_pids);
        exit(0);
    }
}

// Handler για την αναγέννηση παιδιών (Health Check)
void sigchld_handler(int sig) {
    int status;
    pid_t dead_pid;
    
    // Το WNOHANG επιτρέπει στον πατέρα να μην "κολλήσει" αν υπάρχουν πολλά σήματα
    while ((dead_pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            // Βρες ποιο παιδί πέθανε και αντικατάστησέ το
            for (int i = 0; i < num_children; i++) {
                if (child_pids[i] == dead_pid) {
                    printf("[Father] Child %d (PID %d) died. Respawning...\n", i, dead_pid);
                    spawn_child(i);
                    break;
                }
            }
        } else if (WIFSTOPPED(status)) {
            printf("[Father] Child %d stopped. Sending SIGCONT to resume...\n", dead_pid);
            kill(dead_pid, SIGCONT);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <number_of_children>\n", argv[0]);
        return 1;
    }

    num_children = atoi(argv[1]);
    if (num_children <= 0) {
        fprintf(stderr, "Please provide a positive integer.\n");
        return 1;
    }

    child_pids = malloc(num_children * sizeof(pid_t));

    // Σε περίπτωση αποτυχίας μνήμης
    if (child_pids == NULL) {
    perror("Malloc failed");
    return 1;
    }
    // Ρύθμιση Handler για προώθηση σημάτων
    struct sigaction sa_forward;
    sa_forward.sa_handler = forward_signal;
    sigemptyset(&sa_forward.sa_mask);
    sa_forward.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &sa_forward, NULL);
    sigaction(SIGUSR2, &sa_forward, NULL);
    sigaction(SIGTERM, &sa_forward, NULL);

    // Ρύθμιση Handler για SIGCHLD (Αναγέννηση παιδιών)
    struct sigaction sa_chld;
    sa_chld.sa_handler = sigchld_handler;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP; 
    // Σημείωση: SA_RESTART για να μην διακόπτονται οι system calls
    sigaction(SIGCHLD, &sa_chld, NULL);

    // Αρχική δημιουργία των n παιδιών
    for (int i = 0; i < num_children; i++) {
        spawn_child(i);
    }

    printf("[Father] All children spawned. Process tree is ready.\n");

    // εκτύπωση δέντρου
    print_process_tree();
    
    // Ο πατέρας παραμένει ζωντανός για να συντονίζει
    while (1) {
        pause();
    }

    return 0;
}
