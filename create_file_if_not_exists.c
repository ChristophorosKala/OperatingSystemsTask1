#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

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

    char *pathname = argv[1];

    // Έλεγχος αν υπάρχει ήδη το αρχείο
    struct stat st;
    if (stat(pathname, &st) == 0) {
        printf("Error: %s already exists\n", pathname);
        return 1;
    }
    if (errno != ENOENT) {
        perror("stat");
        return 1;
    }

    // Δημιουργία αρχείου
    int fd = open(pathname, O_WRONLY | O_CREAT, 0644);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    // Κλείσιμο αρχείου 
    if (close(fd) == -1) {
        perror("close");
        return 1;
    }

    printf("OK: created file %s\n", pathname);
    return 0;
}
