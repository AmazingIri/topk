#include <unistd.h>
#include "generator.h"

int main(int argc, char *argv[]) {
	int opt;
    std::string input_file_name = "input.txt";
	int Megabytes = 1024;

    while ((opt = getopt(argc, argv, "i:M:")) != -1) {
        switch (opt) {
        case 'i':
            input_file_name = std::string(optarg);
        break;
		case 'M':
			if (!optarg) {
				fprintf(stderr, "invalid option\n");
				exit(EXIT_FAILURE);
			}
            Megabytes = atoi(optarg);
        break;
        default:
            printf("Usage: %s -i input_file_name -M file_size_in_MB\n", argv[0]);
            return -1;
        break;
        }
    }

	GenerateInputURLFile(input_file_name, 1024ull*1024*Megabytes, 5, 20);
	return 0;
}
