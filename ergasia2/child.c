/* * ======================================================================================================================
 * ΑΡΧΕΙΟ: child.c
 * ΠΕΡΙΓΡΑΦΗ: Υλοποίηση της διεργασίας-παιδιού για την εργασία στα Λειτουργικά Συστήματα.
 * 1. Η διεργασία ξεκινάει, αρχικοποιεί το val=0 και καταγράφει το start_time.
 * 2. Χρησιμοποιεί τη δομή sigaction για να ορίσει handlers στα παρακάτω σήματα:
 * - SIGUSR1: Αυξάνει την μεταβλητή val κατά 1.
 * - SIGUSR2: Μειώνει την μεταβλητή val κατά 1.
 * - SIGALRM: Εκτυπώνει status (PID, Val, Time) κάθε 10 δευτερόλεπτα.(Μας ζητάνε 10 αλλά επειδή θα κάνουμε αλλαγές στα 
 * παιδία με χρήση του terminal, θα ήταν καλύτερο να το κάνουμε 15)
 * - SIGTERM: Εxit της διεργασίας όταν τελειώσει το πρόγραμμα ή τερματιστεί μέσω terminal.
 * 3. Η διεργασία παραμένει σε αναμονή μέσω της pause(), ώστε να μην σπαταλάει κύκλους της CPU μέχρι να έρθει κάποιο signal.
 * *  Αν δεν υπήρχε το while(1), το πρόγραμμα θα εκτελούσε την alarm(10), θα έφτανε στο return 0 και θα έκλεινε αμέσως.
 * * ΠΡΟΣΟΧH: Όταν ο πατέρας θα καλέσει την fork() θα πρέπει μετά να αλλάξουμε τις ιδιότητες του αντιγράφου με χρήση
 * * της execv(). Εκεί θα πρέπ να πάρε όρισμα το το όνομα του αρχάιου όπως είναι(νομίζω !!) 
 * * Σημείωση: Στην γραμμή 45 έχω βάλει το πρώτο alarm, ώστε να στείλει το λειτοπυργικό το πρώτο σήμα SIGALRM, και μετά μόλις
 * * έρθει το πρώτο SIGALRM ξαναστέλνει καθε 10 sec, ώστε να κάνουμε συχνό update το status του παδιού.
 * =========================================================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

int val = 0;
time_t start_time; // Η ώρα που ξεκίνησε το παιδί.

// Συνάρτηση χειρισμού σημάτων 
void handle_signal(int sig);

int main() {
    start_time = time(NULL); // Πάρε την τρέχουσα ώρα τώρα και αποθήκευσέ την στη μεταβλητή start_time

    struct sigaction act;
    act.sa_handler = handle_signal;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_RESTART;

    sigaction(SIGUSR1, &act, NULL);
    sigaction(SIGUSR2, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGALRM, &act, NULL);

    // Προγραμματισμός του πρώτου alarm
    alarm(10);

    while(1) {
        pause();
    }

    return 0;
}

void handle_signal(int sig) {
    if (sig == SIGUSR1) {
        val++;
    } 
    else if (sig == SIGUSR2) {
        val--;
    } 
    else if (sig == SIGTERM) {
        printf("Παιδί με ID %d Τερματίζει\n", getpid());
        exit(0);
    } 
    else if (sig == SIGALRM) {
        int alive_time = (int)(time(NULL) - start_time); //Πόσα δευτερόλεπτα πέρασαν απο την "γεννήση" το παιδί
        printf("Παιδί με ID %d, Τιμή: %d, Χρόνος εκτέλεσης: %d sec\n", getpid(), val, alive_time);
        alarm(10);
    }
}
