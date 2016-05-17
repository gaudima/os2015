#include <unistd.h>
#include <errno.h>

int main(int argc, char ** argv) {
	char buf[256];
	ssize_t bytesRead = -1;
	while(bytesRead != 0) {
		bytesRead = read(STDIN_FILENO, (void *)buf, 256);
		if(bytesRead > 0) {
			ssize_t bytesWritten = 0;
			while(bytesWritten != bytesRead) {
				ssize_t bytes = write(STDOUT_FILENO, (void *)(buf + bytesWritten), bytesRead - bytesWritten);
				if(bytes == -1) {
					perror("Error writing to stdout");
					return 1;
				}
				bytesWritten += bytes;

			}
		} else if(bytesRead == -1) {
			perror("Error reading from stdin");
			return 1;
		}
	}
	return 0;
}