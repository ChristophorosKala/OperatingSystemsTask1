
#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <cstring>

int main() {
    // --- ΡΥΘΜΙΣΗ ΠΕΙΡΑΜΑΤΟΣ ---
    // Αλλαξε τον αριθμό από 1 έως 5 για να δεις διαφορετικά Actions:
    // 1: Term, 2: Ign, 3: Core, 4: Stop, 5: Cont
    int choice = 1; 

    pid_t pid = fork();

    if (pid < 0) {
        perror("Fork failed");
        return 1;
    }

    if (pid == 0) {
        // ΚΩΔΙΚΑΣ ΠΑΙΔΙΟΥ
        std::cout << "[Παιδί] Ξεκινάω τις εκτυπώσεις (PID: " << getpid() << ")..." << std::endl;
        for (int i = 1; i <= 20; ++i) {
            std::cout << "[Παιδί] Εκτύπωση σελίδας " << i << "..." << std::endl;
            sleep(1);
        }
        std::cout << "[Παιδί] Ολοκλήρωσα όλες τις εκτυπώσεις!" << std::endl;
        return 0;
    } else {
        // ΚΩΔΙΚΑΣ ΠΑΤΕΡΑ
        sleep(3); // Περιμένει 3 δευτερόλεπτα πριν δράσει

        int signal_to_send;
        std::string action_name;

        switch(choice) {
            case 1: signal_to_send = SIGTERM; action_name = "Term (Terminate)"; break;
            case 2: signal_to_send = SIGCHLD; action_name = "Ign (Ignore)"; break;
            case 3: signal_to_send = SIGQUIT; action_name = "Core (Dump Core)"; break;
            case 4: signal_to_send = SIGSTOP; action_name = "Stop (Pause)"; break;
            case 5: signal_to_send = SIGCONT; action_name = "Cont (Continue)"; break;
            default: signal_to_send = SIGTERM;
        }

        std::cout << "\n[Πατέρας] Επιλογή: " << action_name << std::endl;
        std::cout << "[Πατέρας] Στέλνω το σήμα στο παιδί..." << std::endl;
        
        kill(pid, signal_to_send);

        // Ειδική περίπτωση για το Cont: Πρέπει πρώτα να το σταματήσουμε για να δούμε αν συνεχίζει
        if (choice == 5) {
            kill(pid, SIGSTOP); // Το σταματάμε
            sleep(2);
            std::cout << "[Πατέρας] Τώρα στέλνω SIGCONT για να συνεχίσει!" << std::endl;
            kill(pid, SIGCONT);
        }

        // Έλεγχος κατάστασης (Status)
        int status;
        waitpid(pid, &status, WUNTRACED | WCONTINUED);

        if (WIFEXITED(status)) {
            std::cout << "[Πατέρας] Αποτέλεσμα: Το παιδί βγήκε κανονικά." << std::endl;
        } else if (WIFSIGNALED(status)) {
            std::cout << "[Πατέρας] Αποτέλεσμα: Το παιδί σκοτώθηκε από σήμα " << WTERMSIG(status) << std::endl
