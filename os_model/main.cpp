#include <iostream>
#include <vector>
#include <map>
#include <functional>
#include <memory>
using namespace std;

enum tag_t {
    READ,
    WRITE,
    FORK,
    MMAP,
    MMAP_STORE,
    MMAP_LOAD,
    EXIT
};

struct ret_t {
    tag_t tag;
    vector<string> args;
    function<ret_t(vector<string>)> cont;
};

struct proc_t {
    vector<string> args;
    function<ret_t(vector<string>)> prog;
};

void kernel(function<ret_t(vector<string>)> main, vector<string> args, vector<string> stdin) {
    vector<proc_t> processes;
    vector<uint8_t> shared_mem;
    processes.push_back((proc_t) { args: args, prog: main });
    while(!processes.empty()) {
        proc_t proc = processes.back();
        processes.pop_back();
        ret_t ret = proc.prog(proc.args);
        switch(ret.tag) {
            case READ: {
                string s = stdin[0];
                stdin.erase(stdin.begin());
                processes.push_back((proc_t) { args: {s}, prog: ret.cont });
                break;
            }
            case WRITE: {
                cout << "STDOUT: " << ret.args[0];
                processes.push_back((proc_t) { args: {}, prog: ret.cont });
                break;
            }
            case MMAP: {
                size_t last_size = shared_mem.size();
                shared_mem.resize(last_size + stoll(ret.args[0]));
                processes.push_back((proc_t) { args: {to_string(last_size)}, prog: ret.cont});
                break;
            }
            case MMAP_STORE: {
                size_t addr = stoll(ret.args[0]);
                for(size_t i = 0; i < ret.args[1].size(); i++) {
                    shared_mem[addr + i] = ret.args[1][i];
                }
                processes.push_back((proc_t) { args: {}, prog: ret.cont});
                break;
            }
            case MMAP_LOAD: {
                size_t addr = stoll(ret.args[0]);
                string data;
                for(size_t i = addr; i < addr + stoll(ret.args[1]); i++) {
                    data += (char)(shared_mem[i]);
                }
                processes.push_back((proc_t) { args: {data}, prog: ret.cont});
                break;
            }
            case FORK: {
                processes.push_back((proc_t) { args: {"1"}, prog: ret.cont });
                processes.push_back((proc_t) { args: {"0"}, prog: ret.cont });
                break;
            }
            case EXIT: {
                cout << "Exiting with code: " << ret.args[0] << "\n";
                break;
            }
        }
    }
}

ret_t hello(vector<string> args) {
    map<string, string>  *env = new map<string, string>;

    auto hello_mmap_exit_child = [](vector<string> args) {
        return (ret_t) {tag: EXIT, args: {"0"}, cont: NULL};
    };

    auto hello_mmap_exit_parent = [=](vector<string> args) {
        delete env;
        return (ret_t) {tag: EXIT, args: {"0"}, cont: NULL};
    };

    auto hello_mmap_print = [=](vector<string> args) {
        return (ret_t) {tag: WRITE, args: {args[0] + "\n"}, cont: hello_mmap_exit_parent};
    };

    auto hello_mmap_load = [=](vector<string> args) {
        return (ret_t) {tag: MMAP_LOAD, args: {(*env)["mmap_addr"], "11"}, cont: hello_mmap_print};
    };

    auto hello_mmap_store = [=](vector<string> args) {
        if(args[0] ==  "1") {
            return (ret_t) {tag: MMAP_STORE, args: {(*env)["mmap_addr"], "Parent12"}, cont: hello_mmap_load};
        } else {
            return (ret_t) {tag: MMAP_STORE, args: {to_string(stoll((*env)["mmap_addr"]) + 6), "Child"}, cont: hello_mmap_exit_child};
        }
    };

    auto hello_fork = [=](vector<string> args) {
        (*env)["mmap_addr"] = args[0];
        return (ret_t) {tag: FORK, args: {}, cont: hello_mmap_store};
    };

    auto hello_mmap = [=](vector<string> args) {
        return (ret_t) {tag: MMAP, args: {"20"}, cont: hello_fork};
    };

    return hello_mmap(args);
}


int main() {
    kernel(hello, {}, {});
    return 0;
}