#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

void sigaction_handler(int signum, siginfo_t *info, void *usrcontext) {
	char name[] = "SIGUSR_";
	if(signum == SIGUSR1) {
		name[6] = '1';
	} else if(signum == SIGUSR2) {
		name[6] = '2';
	}
	printf("%s from %d\n", name, info->si_pid);
	exit(0);
}

int main(int argc, char** argv) {
	struct sigaction new_sigaction = (struct sigaction) {
		.sa_sigaction = sigaction_handler,
		.sa_flags = SA_SIGINFO
	};
	if(sigaction(SIGUSR1, &new_sigaction, NULL) != 0 || sigaction(SIGUSR2, &new_sigaction, NULL) != 0) {
		perror("Can not assign handlers");
	}
	sleep(10);
	printf("No signlas were caught\n");
	return 0;
}