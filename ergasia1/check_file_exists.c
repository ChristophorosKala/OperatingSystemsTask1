#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>



int main(int argc, char *argv[])
{
	// --help: usage 
   	if (argc == 2 && strcmp(argv[1], "--help") == 0) {
       		printf("Usage: ./a.out filename\n");
        	return 0;
    	}
	//  λάθος αριθμός ορισμάτων 
    	if (argc != 2) {
        	printf("Usage: ./a.out filename\n");
        	return 1;
    	}

	// Έλεγχος αν υπάρχει ήδη το αρχείο 
	char *pathname = argv[1];
   	struct stat st;
	if(stat(pathname, &st) == 0)
	{
	
		printf("Error: %s already exists\n", pathname);
		return 1;
	}
	else{
		if(errno != ENOENT)
		{
			perror("stat");
			return 1;
		}
	}
		
}
