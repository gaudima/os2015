#include <iostream>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <memory.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/stat.h>
#include <signal.h>
#include <syslog.h>

using namespace std;

class io {
private:
    char buf[512];
public:
    int from;
    int to;

    io(int from, int to) {
        this->from = from;
        this->to = to;
    }

    int transfer() {
        ssize_t bytesRead = read(from, buf, 512);
        if(bytesRead > 0) {
            ssize_t bytesWritten = 0;
            while(bytesWritten != bytesRead) {
                ssize_t bytes = write(to, buf + bytesWritten, bytesRead - bytesWritten);
                if(bytes == -1) {
                    return -1;
                }
                bytesWritten += bytes;
            }
        } else {
            return -1;
        }
        return 0;
    }

    int close_all(int epoll_fd) {
        if(epoll_ctl(epoll_fd, EPOLL_CTL_DEL, from, NULL) < 0 || epoll_ctl(epoll_fd, EPOLL_CTL_DEL, to, NULL) < 0) {
            return -1;
        }
        if(close(from) < 0 || close(to) < 0) {
            return -1;
        }
        return 0;
    }
};

void lerror(const char *msg) {
    syslog(LOG_ERR, "%s: %s", msg, strerror(errno));
}

void sigint_handler(int sig) {
    if(sig == SIGINT) {
        if(remove("/tmp/rshd.pid") < 0) {
            lerror("cant remove /tmp/rshd.pid");
        }
        syslog(LOG_DEBUG, "stopping daemon");
        closelog();
        exit(0);
    }
}

int demonize() {
    int pid = fork();
    if(pid == 0) {
        if(setsid() < 0)
        {
            return -1;
        }
        signal(SIGCHLD, SIG_IGN);
        signal(SIGINT, sigint_handler);
        pid = fork();
        if(pid == 0) {
            if(chdir("/") < 0 ||
                close(STDIN_FILENO) < 0 ||
                close(STDOUT_FILENO) < 0 ||
                close(STDERR_FILENO) < 0 ||
                umask(0) < 0) {
                return -1;
            }
        } else if(pid < 0) {
            return -1;
        } else {
            char pidstr[10];
            sprintf(pidstr, "%d", pid);
            int fd = open("/tmp/rshd.pid", O_WRONLY | O_CREAT | O_TRUNC);
            if(fd < 0) {
                perror("cant open /tmp/rshd.pid for writing");
                exit(-1);
            }
            if(write(fd, pidstr, strlen(pidstr)) < 0) {
                perror("cant write pid");
                exit(-1);
            }
            close(fd);
            exit(0);
        }
    } else if(pid == -1) {
        return -1;
    } else {
        exit(0);
    }
    return 0;
}

int main(int argc, char** argv) {
    int port = 1234;
    if(argc > 1) {
        if(strcmp(argv[1], "stop") == 0) {
            int fd = open("/tmp/rshd.pid", O_RDONLY);
            if(fd < 0) {
                perror("cant open /tmp/rshd.pid for reading");
                exit(-1);
            }
            char pidstr[10];
            int len = read(fd, pidstr, 10);
            if(len < 0) {
                perror("cant read pid");
            }
            if(len >= 10) {
                printf("pid too long\n");
                exit(-1);
            }
            close(fd);
            pidstr[len] = '\0';
            if(kill(atoi(pidstr), SIGINT) < 0) {
                perror("cant kill daemon");
            } else {
                printf("killed daemon\n");
            }
            exit(-1);
        } else {
            for(int i = 0; i < strlen(argv[1]); i++) {
                char digit = argv[1][i];
                if(digit < '0' || digit > '9') {
                    printf("argument is not a valid port number\n");
                    exit(-1);
                }
            }
            port = atoi(argv[1]);
        }
    }
    if(demonize() < 0) {
        perror("cant demonize");
        exit(-1);
    }
    openlog("rshd", LOG_PID | LOG_CONS, LOG_DAEMON);

    const int EPOLL_MAX_SIZE = 100;
    int sock = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);

    if(sock < 0) {
        lerror("cant create socket");
        exit(-1);
    }

    int epoll_fd = epoll_create1(EPOLL_CLOEXEC);

    syslog(LOG_DEBUG, "port: %d", port);

    sockaddr_in serv_adrr;
    bzero(&serv_adrr, sizeof(serv_adrr));
    serv_adrr.sin_family = AF_INET;
    serv_adrr.sin_addr.s_addr = INADDR_ANY;
    serv_adrr.sin_port = htons(port);

    if(bind(sock, (sockaddr *) &serv_adrr, sizeof(serv_adrr)) < 0) {
        lerror("cant bind socket");
        exit(-1);
    }

    listen(sock, 5);

    epoll_event epoll_ev;
    epoll_ev.events = EPOLLIN | EPOLLET | EPOLLPRI;
    epoll_ev.data.fd = sock;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock, &epoll_ev) < 0) {
        lerror("cant add sock to epoll");
        exit(-1);
    }

    sockaddr_in client_addr;
    socklen_t client_addr_size = sizeof(client_addr);

    epoll_event events[EPOLL_MAX_SIZE];

    while(true) {
        int num_fds = epoll_wait(epoll_fd, events, EPOLL_MAX_SIZE, -1);
        for(int i = 0; i < num_fds; i++) {
            if(events[i].data.fd == sock) {
                syslog(LOG_DEBUG, "changes on sock");
                int client_sock = accept4(sock, (sockaddr *) &client_addr, &client_addr_size, SOCK_CLOEXEC);
                if (client_sock < 0) {
                    lerror("cant accept client");
                    continue;
                }

                int pty_master_fd = posix_openpt(O_RDWR | O_CLOEXEC);

                if (pty_master_fd < 0) {
                    lerror("cant pty");
                    continue;
                }

                int rc = grantpt(pty_master_fd);
                if(rc < 0) {
                    lerror("cant grantpt");
                    continue;
                }

                rc = unlockpt(pty_master_fd);
                if(rc < 0) {
                    lerror("cant unlockpt");
                    continue;
                }


                io *c2s = new io(client_sock, pty_master_fd);
                io *s2c = new io(pty_master_fd, client_sock);

                epoll_ev.data.ptr = c2s;
                epoll_ev.events = EPOLLIN | EPOLLRDHUP;
                if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_sock, &epoll_ev) < 0) {
                    lerror("cant add client->pty to epoll");
                    c2s->close_all(epoll_fd);
                    continue;
                }

                epoll_ev.data.ptr = s2c;
                epoll_ev.events = EPOLLIN;
                if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pty_master_fd, &epoll_ev) < 0) {
                    lerror("cant add pty->client to epoll");
                    s2c->close_all(epoll_fd);
                    continue;
                }
                syslog(LOG_DEBUG, "client accepted");
                int pid = fork();
                if (pid < 0) {
                    lerror("cant fork to exec sh");
                    c2s->close_all(epoll_fd);
                    continue;
                } else if (pid == 0) {
                    if(setsid() < 0) {
                        lerror("cant setsid of sh");
                        exit(-1);
                    }
                    int pty_slave_fd = open(ptsname(pty_master_fd), O_RDWR | O_CLOEXEC);
                    if(pty_slave_fd < 0) {
                        lerror("cant open slave pseudo terminal end");
                        exit(-1);
                    }
                    termios pty_settings;
                    if(tcgetattr(pty_slave_fd, &pty_settings) < 0) {
                        lerror("cant get pty settings");
                        exit(-1);
                    }
                    pty_settings.c_lflag &= ~ECHO;
                    pty_settings.c_lflag |= ISIG;
                    if(tcsetattr(pty_slave_fd, TCSANOW, &pty_settings) < 0) {
                        lerror("cant set pty settings");
                        exit(-1);
                    }
                    dup2(pty_slave_fd, STDIN_FILENO);
                    dup2(pty_slave_fd, STDOUT_FILENO);
                    dup2(pty_slave_fd, STDERR_FILENO);
                    execl("/bin/sh", "sh", NULL);
                }
            } else {
                syslog(LOG_DEBUG, "changes on other fds");
                io *trans = (io *)(events[i].data.ptr);
                if(events[i].events & EPOLLRDHUP) {
                    syslog(LOG_DEBUG, "client disconnected");
                    if(trans->close_all(epoll_fd) < 0) {
                        lerror("cant close fds");
                    }
                    delete trans;
                    continue;
                }
                if(trans->transfer() < 0) {
                    lerror("error transferring data");
                }
            }
        }
    }
    return 0;
}