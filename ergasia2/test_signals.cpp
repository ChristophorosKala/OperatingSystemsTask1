#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <cstring>

int main() {
    // --- ΡΥΘΜΙΣΗ ΠΕΙΡΑΜΑΤΟΣ ---
    // 1: Term (Τερματισμός)
    // 2: Ign  (Αγνόηση - το παιδί συνεχίζει)
    // 3: Core (Τερματισμός με Core Dump)
    // 4: Stop (Πάγωμα - Χρησιμοποίησέ το για το πείραμα με το 2ο τερματικό)
    // 5: Cont (Συνέχεια)
    int choice = 4; 

    pid_t pid = fork();

    if (pid < 0) {
        perror("Fork failed");
        return 1;
    }

    if (pid == 0) {
        // --- ΚΩΔΙΚΑΣ ΠΑΙΔΙΟΥ ---
        std::cout << "[Παιδί] Ξεκινάω τις εκτυπώσεις (PID: " << getpid() << ")..." << std::endl;
        for (int i = 1; i <= 20; ++i) {
            std::cout << "[Παιδί] Εκτύπωση σελίδας " << i << "..." << std::endl;
            sleep(1);
        }
        std::cout << "[Παιδί] Ολοκλήρωσα όλες τις εκτυπώσεις!" << std::endl;
        return 0;
    } else {
        // --- ΚΩΔΙΚΑΣ ΠΑΤΕΡΑ ---
        sleep(3); // Περιμένει λίγο να ξεκινήσει το παιδί

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

        // Βρόχος παρακολούθησης του παιδιού
        int status;
        bool child_is_alive = true;

        std::cout << "[Πατέρας] Μπαίνω σε κατάσταση αναμονής για το παιδί..." << std::endl;

        while (child_is_alive) {
            // Το waitpid εδώ "πιάνει" τα πάντα: θάνατο, πάγωμα (Stop) και ξύπνημα (Continue)
            waitpid(pid, &status, WUNTRACED | WCONTINUED);

            if (WIFEXITED(status)) {
                std::cout << "[Πατέρας] Το παιδί βγήκε κανονικά (exit)." << std::endl;
                child_is_alive = false;
            } 
            else if (WIFSIGNALED(status)) {
                std::cout << "[Πατέρας] Το παιδί σκοτώθηκε από σήμα: " << WTERMSIG(status) << std::endl;
                child_is_alive = false;
            } 
            else if (WIFSTOPPED(status)) {
                std::cout << "[Πατέρας] ΕΙΔΟΠΟΙΗΣΗ: Το παιδί μόλις ΣΤΑΜΑΤΗΣΕ (Stopped)." << std::endl;
                std::cout << "[Πατέρας] Περιμένω σήμα SIGCONT από το άλλο τερματικό για να συνεχίσω..." << std::endl;
            } 
            else if (WIFCONTINUED(status)) {
                std::cout << "[Πατέρας] ΕΙΔΟΠΟΙΗΣΗ: Το παιδί ΣΥΝΕΧΙΖΕΙ (Continued)!" << std::endl;
            }
        }

        std::cout << "[Πατέρας] Το παιδί τελείωσε πλήρως. Κλείνω κι εγώ." << std::endl;
    }

    return 0;
}
