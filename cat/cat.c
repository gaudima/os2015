#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>

int main(int argc, char ** argv) {
	if(argc < 2) {
		fprintf(stderr, "Not enough argumnets\n");
		return -1;
	}
	int file = open(argv[1], O_RDONLY);
	if(file == -1) {
		perror("Error openeing file");
		return -1;
	}
	char buf[256];
	ssize_t bytesRead = -1;
	while(bytesRead != 0) {
		bytesRead = read(file, (void *)buf, 256);
		if(bytesRead > 0) {
			ssize_t bytesWritten = 0;
			while(bytesWritten != bytesRead) {
				ssize_t bytes = write(STDOUT_FILENO, (void *)(buf + bytesWritten), bytesRead - bytesWritten);
				if(bytes == -1) {
					if(errno != EINTR) {
						perror("Error writing to stdout");
						return -1;	
					}
				} else {
					bytesWritten += bytes;
				}
			}
		} else if(bytesRead == -1) {
			if(errno != EINTR) {
				perror("Error reading from stdin");
				return -1;
			}
		}
	}
	if(close(file) == -1) {
		perror("Error closing file");
		return -1;
	}
	return 0;
}
