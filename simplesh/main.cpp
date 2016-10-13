#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <string>
#include <vector>
#include <iostream>

using namespace std;

vector<pid_t> childs;

struct Command {
    string command;
    vector<string> args;
};

struct CmdStr {
    string command;
    string rest;
};

union Pipe {
    int pip[2];
    struct {
        int read;
        int write;
    };
};

void sigintHandler(int sig) {
    if(sig == SIGINT) {
        for(int i = 0; i < childs.size(); i++) {
            kill(childs[i], SIGINT);
        }
        write(STDOUT_FILENO, "\n", 1);
    }
}

string readCommands(int desc, int bufSize) {
    string ret;
    char buf[bufSize];
    while(true) {
        ssize_t bytesRead = read(desc, buf, bufSize);
        if(bytesRead == -1) {
            perror("cant read commands");
            exit(-1);
        }
        if(bytesRead == 0) {
            break;
        }
        ret.append(buf, bytesRead);
        for(int i = 0; i < bytesRead; i++) {
            if(buf[i] == '\n') {
                return ret;
            }
        }
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

CmdStr splitCommand(string s) {
    CmdStr ret = {"", ""};
    for(int i = 0; i < s.length(); i++) {
        if(s[i] == '\n') {
            ret.command.append(s, 0, i);
            ret.rest.append(s, i + 1, s.length() - i - 1);
            return ret;
        }
    }
    ret.command.append(s);
}

vector<Command> tokenize(string &str) {
    vector<Command> commands;
    size_t firsti = 0;
    for(size_t i = 0; i < str.size(); i++) {
        Command tmp;
        string tmps;
        if((str[i] == '|' || i == str.size() - 1) && i >= firsti) {
            if(i == str.size() - 1) {
                i = str.size() + 1;
            }
            tmps = trim(str.substr(firsti, i - firsti - 1));
            size_t pos = tmps.find(' ');
            if(pos == string::npos) {
                tmp.command = tmps;
            } else {
                tmp.command = tmps.substr(0, pos);
                string argsS = trim(tmps.substr(pos + 1));
                size_t firstj = 0;
                for(size_t j = 0; j < argsS.size(); j++) {
                    if((argsS[j] == ' ' || j == argsS.size() - 1) && j >= firstj) {
                        if(j == argsS.size() - 1) {
                            j = argsS.size() + 1;
                        }
                        string tmpa = trim(argsS.substr(firstj, j - firstj));
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


string runCommands(vector<Command> &commands, string rest) {
    int in, newIn = STDIN_FILENO;
    Pipe p;
    p.read = -1;
    p.write = -1;
    if(rest.size() > 0) {
        //cout << "here1" << endl;
        if (pipe(p.pip) < 0) {
            perror("cant create pipe");
            exit(-1);
        }
        write(p.write, rest.c_str(), rest.length());
        close(p.write);
        newIn = p.read;
    }
    int out;
    for (int i = 0; i < commands.size(); i++) {
        in = newIn;
        if (i != commands.size() - 1) {
            Pipe pipes;
            if (pipe2(pipes.pip, O_CLOEXEC) < 0) {
                perror("cant create pipe");
                break;
            };
            newIn = pipes.read;
            out = pipes.write;
        } else {
            out = STDOUT_FILENO;
        }
        pid_t pid = fork();
        if (pid == -1) {
            perror("cant fork");
            break;
        }
        if (pid > 0) {
            if (in != STDIN_FILENO && in != p.read) close(in);
            if (out != STDOUT_FILENO) close(out);
            childs.push_back(pid);
        } else {
            dup2(in, STDIN_FILENO);
            dup2(out, STDOUT_FILENO);
            char *args[commands[i].args.size() + 2];
            args[0] = (char *)commands[i].command.c_str();
            for(int j = 1; j <= commands[i].args.size(); j++) {
                args[j] = (char *)commands[i].args[j - 1].c_str();
            }
            args[commands[i].args.size() + 1] = NULL;
            execvp(commands[i].command.c_str(), args);
        }
    }

    int status;
    for (int i = 0; i < childs.size(); i++) {
        waitpid(childs[i], &status, 0);
    }
    childs.clear();
    string ret = "";
    if(rest.size() > 0) {
        //cout << "here" << endl;
        ret = readCommands(p.read, 256);
        close(p.read);
    }
    return ret;
}

int main(int argc, char **argv) {
    string cmd = "";
    signal(SIGINT, sigintHandler);
    while(true) {
        write(STDOUT_FILENO, "$ ", 2);
        string newCmd = readCommands(STDIN_FILENO, 256 - cmd.size());

        cmd.append(newCmd);
        //cmd = newCmd;
        if(cmd.size() == 0) {
            write(STDOUT_FILENO, "\n", 1);
            break;
        }

        CmdStr cStr = splitCommand(cmd);
        //cout << cStr.rest.length() << endl;
        vector<Command> commands = tokenize(cStr.command);
        cmd = runCommands(commands, cStr.rest);
        //cout << "cmd: " << cmd << endl;
    }
    return 0;
}