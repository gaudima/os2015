#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <signal.h>
#include <string>
#include <vector>
using namespace std;

int pid;

struct Command {
    string command;
    vector<string> args;
};

void sigintHandler(int sig) {
    if(sig == SIGINT) {
        kill(-1 * pid, SIGINT);
        write(STDOUT_FILENO, "\n", 1);
    }
}

string readCommands() {
    string ret;
    char buf[256];
    ssize_t bytesRead;
    while(true) {
        bytesRead = read(STDIN_FILENO, buf, 256);
        if(bytesRead == -1) {
            perror("cant read commands");
            exit(-1);
        }
        if(bytesRead == 0) {
            write(STDOUT_FILENO, "\n", 1);
            exit(0);
        }
        bool shouldExit = false;
        for(ssize_t i = 0; i < bytesRead; i++) {
            if(buf[i] == '\n') {
                shouldExit = true;
                ret.append(buf, i);
                break;
            }
        }
        if(shouldExit) {
            break;
        }
        ret.append(buf, bytesRead);
    }
    return ret;
}

string trimLeft(string str) {
    str.erase(0, str.find_first_not_of(" \n\r\t"));
    return str;
}

string trimRight(string str) {
    str.erase(str.find_last_not_of(" \n\r\t") + 1);
    return str;
}

string trim(string str) {
    return trimLeft(trimRight(str));
}


vector<Command> tokenize(string &str) {
    vector<Command> commands;
    size_t firsti = 0;
    for(size_t i = 0; i < str.size(); i++) {
        Command tmp;
        string tmps;
        if((str[i] == '|' | i == str.size() - 1) && i >= firsti) {
            if(i == str.size() - 1) {
                i = str.size() + 1;
            }
            tmps = trim(str.substr(firsti, i - firsti - 1));
            size_t pos = tmps.find(' ');
            if(pos == string::npos) {
                tmp.command = tmps;
            } else {
                tmp.command = tmps.substr(0, pos);
                string argsS = tmps.substr(pos + 1);
                size_t firstj = 0;
                for(size_t j = 0; j < argsS.size(); j++) {
                    if((argsS[j] == ' ' | j == argsS.size() - 1) && j >= firstj) {
                        if(j == argsS.size() - 1) {
                            j = argsS.size() + 1;
                        }
                        string tmpa = trim(argsS.substr(firstj, j - firstj - 1));
                        //cout << tmpa << endl;
                        tmp.args.push_back(tmpa);
                        firstj = j + 1;
                    }
                }
            }
            commands.push_back(tmp);
            firsti = i + 1;
        }
    }
    return commands;
}


void runCommands(vector<Command> &commands) {
    int pip[2];
    int in = 0;

    for(int i = 0; i < commands.size(); i++) {
        pipe(pip);
        pid = fork();
        if(pid == 0) {
            dup2(in, STDIN_FILENO);
            if(i != commands.size() - 1) {
                dup2(pip[1], STDOUT_FILENO);
            }
            close(pip[0]);
            char *args[commands[i].args.size() + 2];
            args[0] = (char *)commands[i].command.c_str();
            for(int j = 1; j <= commands[i].args.size(); j++) {
                args[j] = (char *)commands[i].args[j - 1].c_str();
            }
            args[commands[i].args.size() + 1] = NULL;
            execvp(commands[i].command.c_str(), args);
        } else {
            setpgid(pid, 0);
            wait(NULL);
            close(pip[1]);
            in = pip[0];
        }
    }
}

int main(int argc, char **argv) {
    string str = "foo";
    signal(SIGINT, sigintHandler);
    while(str.size() != 0) {
        write(STDOUT_FILENO, "$ ", 2);
        string str = readCommands();
        vector<Command> commands = tokenize(str);
        runCommands(commands);
    }
    return 0;
}