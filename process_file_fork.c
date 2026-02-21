#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define N 64

int main(int argc, char *argv[])
{
    // --help
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        printf("Usage: ./a.out filename\n");
        return 0;
    }

    // Λάθος αριθμός ορισμάτων
    if (argc != 2) {
        printf("Usage: ./a.out filename\n");
        return 1;
    }

    char *filename = argv[1];

    // Έλεγχος αν υπάρχει ήδη το αρχείο
    struct stat st;
    if (stat(filename, &st) == 0) {
        printf("Error: %s already exists\n", filename);
        return 1;
    }
    if (errno != ENOENT) {
        perror("stat");
        return 1;
    }

    // Δημιουργία αρχείου
    int fd = open(filename, O_WRONLY | O_CREAT, 0644);
    if (fd == -1) {
        perror("open");
        return 1;
    }
    
    // Αποτυχία δημιουργίας παιδιού
    pid_t pid = fork(); 
    if(pid == -1) {
        perror("fork");
        close(fd);
        return 1;
    }
    
    char buffer[N];
    int len;
    // Επιτυχία δημιουργίας Παιδιού
    if(pid == 0) { 
        len = snprintf(buffer, N, "[CHILD] getpid()= %d, getppid()= %d\n", getpid(), getppid());
        
        // Γράψιμο στο αρχείο
        if(write(fd, buffer, len) == -1) {
            perror("write");
            close(fd);
            exit(1); 
        }
        
        close(fd);
        exit(0);
    }
	//Δημιουργία Γονέα
    else { 
        //Αναστολή τη διεργασίας γονέα μέχρι να τερματίσει το παιδί
        wait(NULL);
        
        len = snprintf(buffer, N, "[PARENT] getpid()= %d, getppid()= %d\n", getpid(), getppid());

        if (write(fd, buffer, len) == -1) {
            perror("write");
            close(fd);
            exit(1);
        }

        close(fd);
    }

    return 0;
}
