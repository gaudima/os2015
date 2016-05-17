#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>

bool inHandler = false;

void sigaction_handler(int signum, siginfo_t *info, void *usrcontext) {
	if(!inHandler) {
		inHandler = true;
		printf("%d from %d\n", signum, info->si_pid);
		exit(0);
	}
}

int main(int argc, char** argv) {
	struct sigaction new_sigaction = (struct sigaction) {
		.sa_sigaction = sigaction_handler,
		.sa_flags = SA_SIGINFO
	};
	int i;
	for(i = 1; i < 32; i++) {
		if(i != SIGKILL && i != SIGSTOP) {
			if(sigaction(i, &new_sigaction, NULL) != 0) {
				printf("Can not assign handler to signal %d\n", i);
			}
		}
	}
	sleep(10);
	inHandler = true;
	printf("No signlas were caught\n");
	return 0;
}