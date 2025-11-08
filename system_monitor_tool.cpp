#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <unistd.h>
#include <signal.h>
#include <pwd.h>
#include <ncurses.h>
using namespace std;
namespace fs = std::filesystem;

struct Process {
    int pid;
    string user, name;
    float cpu, memMB;
};

unsigned long long getTotalCPU() {
    ifstream f("/proc/stat");
    string t; unsigned long long a,b,c,d,e,f_,g,h;
    f >> t >> a >> b >> c >> d >> e >> f_ >> g >> h;
    return a+b+c+d+e+f_+g+h;
}

unsigned long long getProcCPU(int pid) {
    ifstream f("/proc/" + to_string(pid) + "/stat");
    if (!f) return 0;
    string s; unsigned long long u, st;
    for (int i = 0; i < 13; i++) f >> s;
    f >> u >> st;
    return u + st;
}

float getMemMB(int pid) {
    ifstream f("/proc/" + to_string(pid) + "/status");
    string k; long v = 0;
    while (f >> k) {
        if (k == "VmRSS:") { f >> v; break; }
        string t; getline(f, t);
    }
    return v / 1024.0;
}

vector<Process> getProcesses(unsigned long long prevT, unsigned long long curT,
                             const vector<pair<int,unsigned long long>>& prevCPU) {
    vector<Process> list;
    for (auto &e : fs::directory_iterator("/proc")) {
        if (!e.is_directory()) continue;
        string pidStr = e.path().filename();
        if (!all_of(pidStr.begin(), pidStr.end(), ::isdigit)) continue;
        int pid = stoi(pidStr);

        ifstream comm(e.path() / "comm");
        string name; getline(comm, name);
        if (name.empty()) continue;

        ifstream status(e.path() / "status");
        string line; uid_t uid = 0; string user = "unknown";
        while (getline(status, line))
            if (line.rfind("Uid:", 0) == 0) {
                istringstream s(line.substr(4)); s >> uid;
                if (auto pw = getpwuid(uid)) user = pw->pw_name;
                break;
            }

        unsigned long long prev = 0;
        for (auto &p : prevCPU) if (p.first == pid) { prev = p.second; break; }

        unsigned long long now = getProcCPU(pid);
        float cpu = curT > prevT ? 100.0 * (now - prev) / (curT - prevT) : 0;
        list.push_back({pid, user, name, cpu, getMemMB(pid)});
    }
    return list;
}

void message(const string &msg, int color) {
    erase();
    attron(COLOR_PAIR(color) | A_BOLD);
    mvprintw(LINES/2, (COLS - msg.size())/2, "%s", msg.c_str());
    attroff(COLOR_PAIR(color) | A_BOLD);
    mvprintw(LINES/2 + 2, (COLS - 28)/2, "[Press any key to retun back]");
    refresh(); getch();
}

int main() {
    initscr(); noecho(); cbreak(); keypad(stdscr, TRUE);
    start_color(); use_default_colors();
    init_pair(1, COLOR_CYAN, -1);
    init_pair(2, COLOR_YELLOW, -1);
    init_pair(3, COLOR_GREEN, -1);
    curs_set(0);

    unsigned long long prevT = getTotalCPU();
    vector<pair<int,unsigned long long>> prevCPU;
    for (auto &p : getProcesses(prevT, getTotalCPU(), prevCPU))
        prevCPU.push_back({p.pid, getProcCPU(p.pid)});
    prevT = getTotalCPU();

    int scroll = 0, sortMode = 0; // 0=PID, 1=CPU, 2=Memory
    timeout(200);

    while (true) {
        unsigned long long curT = getTotalCPU();
        auto procs = getProcesses(prevT, curT, prevCPU);
        prevCPU.clear();
        for (auto &p : procs) prevCPU.push_back({p.pid, getProcCPU(p.pid)});
        prevT = curT;

        // Sorting modes
        if (sortMode == 1)
            sort(procs.begin(), procs.end(), [](auto &a, auto &b){ return a.cpu > b.cpu; });
        else if (sortMode == 2)
            sort(procs.begin(), procs.end(), [](auto &a, auto &b){ return a.memMB > b.memMB; });
        else
            sort(procs.begin(), procs.end(), [](auto &a, auto &b){ return a.pid < b.pid; });

        erase();
        attron(COLOR_PAIR(1)|A_BOLD);
        mvprintw(0, 2, "=== SYSTEM MONITOR TOOL ===");
        attroff(COLOR_PAIR(1)|A_BOLD);

        float cpu = 0, mem = 0;
        for (auto &p : procs) { cpu += p.cpu; mem += p.memMB; }
        attron(COLOR_PAIR(2)|A_BOLD);
        mvprintw(2, 2, "CPU: %.1f%%   Memory: %.1f MB   Processes: %zu", cpu, mem, procs.size());
        attroff(COLOR_PAIR(2)|A_BOLD);

        attron(COLOR_PAIR(1)|A_BOLD);
        mvprintw(4, 2, "%-8s %-14s %10s %16s        %-40s",
                 "PID","USER","CPU(%)","MEMORY(MB)","NAME");
        attroff(COLOR_PAIR(1)|A_BOLD);

        int visible = LINES - 10;
        scroll = max(0, min(scroll, (int)procs.size() - visible));
        for (int i = 0; i < visible && i + scroll < (int)procs.size(); ++i) {
            auto &p = procs[i + scroll];
            string name = p.name.size() > 40 ? p.name.substr(0, 37) + "..." : p.name;
            mvprintw(5 + i, 2, "%-8d %-14s %10.1f %16.1f        %-40s",
                     p.pid, p.user.c_str(), p.cpu, p.memMB, name.c_str());
        }

        int footer = min(LINES - 3, 5 + visible + 1);
        mvhline(footer - 1, 1, ' ', COLS - 2);
        attron(COLOR_PAIR(1)|A_BOLD);
        mvprintw(footer, 2, "[K] Kill  [S] Suspend  [R] Resume  [D] Details  [O] Sort CPU  [M] Sort Mem  [P] Sort PID  [Q] Quit");
        attroff(COLOR_PAIR(1)|A_BOLD);
        refresh();

        int ch = getch();
        if (ch == 'q' || ch == 'Q') break;
        if (ch == KEY_UP && scroll > 0) scroll--;
        if (ch == KEY_DOWN && scroll + visible < (int)procs.size()) scroll++;
        if (ch == 'o' || ch == 'O') sortMode = 1;
        if (ch == 'm' || ch == 'M') sortMode = 2;
        if (ch == 'p' || ch == 'P') sortMode = 0;

        string prompt;
        if (ch=='k') prompt="Enter PID to Kill (B=back): ";
        else if (ch=='s') prompt="Enter PID to Suspend (B=back): ";
        else if (ch=='r') prompt="Enter PID to Resume (B=back): ";
        else if (ch=='d') prompt="Enter PID to View Details (B=back): ";
        else continue;

        timeout(-1); echo();
        mvprintw(footer + 2, 2, "%s", prompt.c_str());
        char input[16]; getnstr(input, 15);
        noecho();

        string in(input);
        if (in == "b" || in == "B" || in.empty()) { timeout(200); continue; }
        int pid = stoi(in);

        if (ch == 'd') {
            erase();
            attron(COLOR_PAIR(1)|A_BOLD);
            mvprintw(0, 2, "Details for PID %d:", pid);
            attroff(COLOR_PAIR(1)|A_BOLD);
            ifstream f("/proc/" + to_string(pid) + "/status");
            string line; int r = 2;
            while (getline(f, line) && r < LINES - 2) mvprintw(r++, 2, "%s", line.c_str());
            mvprintw(LINES - 1, 2, "[Press any key to retun back]");
            refresh(); getch();
        } else {
            int sig = (ch == 'k') ? SIGKILL : (ch == 's') ? SIGSTOP : SIGCONT;
            string act = (ch == 'k') ? "killed" : (ch == 's') ? "suspended" : "resumed";
            if (kill(pid, sig) == 0)
                message("Process " + to_string(pid) + " " + act + ".", 3);
            else
                message("Failed to " + act + " PID " + to_string(pid) + ".", 2);
        }
        timeout(200);
    }
    endwin();
}
