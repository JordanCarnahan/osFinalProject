//Final Project: Thread Scheduling Simulation
//osFinalProject.cpp
#include <iostream>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <random>

using namespace std;
mutex mtx;

//caleb
// scheduler.cpp
// Thread Scheduling Simulator
// Supports: FCFS, SJF (non-preemptive / preemptive), PRIORITY (preemptive), RR (quantum)
// Build: g++ -std=c++17 -O2 scheduler.cpp -o scheduler
// Usage examples:
//   ./scheduler --algo RR --quantum 2 --input example.txt
//   ./scheduler --algo SJF --preemptive --input example.txt
//   ./scheduler --algo FCFS
//
// Input format (whitespace separated): ID arrival burst [priority]
// Example:
// T1 0 5 2
// T2 1 3 1
// Blank lines and lines beginning with # are ignored.

#include <bits/stdc++.h>
using namespace std;

struct TCB {
    string tid;
    int arrival;
    int burst;
    int priority;
    int remaining;
    int start_time;    // -1 if never started
    int completion;    // -1 if not complete

    TCB(string id="", int a=0, int b=0, int p=0)
      : tid(id), arrival(a), burst(b), priority(p),
        remaining(b), start_time(-1), completion(-1) {}
};

struct GanttEntry {
    int start;
    int end;
    string tid; // "idle" for CPU idle
};

static vector<TCB> parse_input_lines(const vector<string>& lines) {
    vector<TCB> res;
    for (auto ln : lines) {
        // trim
        while (!ln.empty() && isspace((unsigned char)ln.back())) ln.pop_back();
        size_t i=0; while (i<ln.size() && isspace((unsigned char)ln[i])) ++i;
        if (i>=ln.size()) continue;
        if (ln[i]=='#') continue;
        istringstream iss(ln);
        string id; int arrival, burst; int priority=0;
        if (!(iss >> id >> arrival >> burst)) {
            cerr << "Bad input line (skipping): " << ln << "\n";
            continue;
        }
        if (iss >> priority) {
            // ok
        }
        res.emplace_back(id, arrival, burst, priority);
    }
    return res;
}

static optional<int> next_arrival_time(const vector<TCB>& tcbs, int now) {
    int best = INT_MAX;
    for (auto &t: tcbs) {
        if (t.arrival > now && t.completion == -1) best = min(best, t.arrival);
    }
    if (best==INT_MAX) return nullopt;
    return best;
}

// Utility to deep-copy TCB list
static vector<TCB> clone_tcbs(const vector<TCB>& in) {
    vector<TCB> out;
    for (auto &t: in) out.push_back(t);
    return out;
}

// Print summary and metrics
static void print_summary(const vector<TCB>& tcbs, const vector<GanttEntry>& gantt) {
    int total_burst=0;
    int completed = 0;
    for (auto &t: tcbs) {
        total_burst += t.burst;
        if (t.completion != -1) ++completed;
    }
    int total_time = gantt.empty() ? 0 : gantt.back().end;
    double utilization = (total_time>0) ? (100.0 * double(total_burst) / double(total_time)) : 0.0;
    double throughput = (total_time>0) ? double(completed) / double(total_time) : 0.0;

    cout << "\nGantt Chart (start,end,tid):\n";
    for (auto &g: gantt) {
        cout << "  [" << setw(3) << g.start << " - " << setw(3) << g.end << "]  " << g.tid << "\n";
    }

    cout << "\nThread\tArrival\tBurst\tCompletion\tWaiting\tTurnaround\n";
    double sum_wait = 0.0, sum_turn = 0.0;
    for (auto t : tcbs) {
        int turn = (t.completion==-1) ? -1 : (t.completion - t.arrival);
        int wait = (turn==-1) ? -1 : (turn - t.burst);
        cout << setw(6) << t.tid << "\t" << setw(6) << t.arrival << "\t" << setw(5) << t.burst
             << "\t" << setw(9) << (t.completion==-1 ? -1 : t.completion)
             << "\t\t" << setw(7) << wait << "\t" << setw(8) << turn << "\n";
        if (turn!=-1) { sum_wait += wait; sum_turn += turn; }
    }
    int n = tcbs.size();
    cout << fixed << setprecision(2);
    cout << "\nAverage waiting time = " << (sum_wait / n) << "\n";
    cout << "Average turnaround time = " << (sum_turn / n) << "\n";
    cout << "CPU Utilization = " << utilization << "%\n";
    cout << "Throughput = " << throughput << " threads/unit-time\n";
}

// FCFS (non-preemptive)
static pair<vector<TCB>, vector<GanttEntry>> schedule_fcfs(const vector<TCB>& tcbs_in) {
    auto tcbs = clone_tcbs(tcbs_in);
    sort(tcbs.begin(), tcbs.end(), [](const TCB& a, const TCB& b){
        if (a.arrival!=b.arrival) return a.arrival < b.arrival;
        return a.tid < b.tid;
    });
    vector<GanttEntry> gantt;
    int now = 0;
    size_t idx = 0;
    while (true) {
        // find next ready
        vector<int> ready_idx;
        for (size_t i=0;i<tcbs.size();++i) {
            if (tcbs[i].arrival <= now && tcbs[i].completion==-1 && tcbs[i].remaining>0) ready_idx.push_back(i);
        }
        if (ready_idx.empty()) {
            // if none ready, advance to next arrival
            auto nxt = next_arrival_time(tcbs, now);
            if (!nxt.has_value()) break;
            int nxtt = nxt.value();
            if (nxtt > now) gantt.push_back({now, nxtt, string("idle")});
            now = nxtt;
            continue;
        }
        // FCFS: choose earliest arrival among ready (their ordering already)
        int choose = ready_idx.front();
        if (tcbs[choose].start_time == -1) tcbs[choose].start_time = now;
        int start = now;
        now += tcbs[choose].remaining;
        tcbs[choose].remaining = 0;
        tcbs[choose].completion = now;
        gantt.push_back({start, now, tcbs[choose].tid});
    }
    return {tcbs, gantt};
}

// SJF non-preemptive
static pair<vector<TCB>, vector<GanttEntry>> schedule_sjf_nonpreemptive(const vector<TCB>& tcbs_in) {
    auto tcbs = clone_tcbs(tcbs_in);
    vector<GanttEntry> gantt;
    int now = 0;
    while (true) {
        vector<int> ready;
        for (size_t i=0;i<tcbs.size();++i) if (tcbs[i].arrival<=now && tcbs[i].completion==-1 && tcbs[i].remaining>0) ready.push_back(i);
        if (ready.empty()) {
            auto nxt = next_arrival_time(tcbs, now);
            if (!nxt.has_value()) break;
            int nxtt = nxt.value();
            if (nxtt > now) gantt.push_back({now, nxtt, string("idle")});
            now = nxtt;
            continue;
        }
        // pick shortest burst (original burst is used)
        sort(ready.begin(), ready.end(), [&](int a, int b){
            if (tcbs[a].burst != tcbs[b].burst) return tcbs[a].burst < tcbs[b].burst;
            if (tcbs[a].arrival != tcbs[b].arrival) return tcbs[a].arrival < tcbs[b].arrival;
            return tcbs[a].tid < tcbs[b].tid;
        });
        int cur = ready.front();
        if (tcbs[cur].start_time == -1) tcbs[cur].start_time = now;
        int start = now;
        now += tcbs[cur].remaining;
        tcbs[cur].remaining = 0;
        tcbs[cur].completion = now;
        gantt.push_back({start, now, tcbs[cur].tid});
    }
    return {tcbs, gantt};
}

// SJF preemptive (SRTF) - simulate in 1-time-unit ticks for correct preemption-by-arrival
static pair<vector<TCB>, vector<GanttEntry>> schedule_sjf_preemptive(const vector<TCB>& tcbs_in) {
    auto tcbs = clone_tcbs(tcbs_in);
    vector<GanttEntry> gantt;
    int now = 0;
    string last_tid = "";
    while (true) {
        vector<int> avail;
        for (size_t i=0;i<tcbs.size();++i) if (tcbs[i].arrival<=now && tcbs[i].completion==-1 && tcbs[i].remaining>0) avail.push_back(i);
        if (avail.empty()) {
            auto nxt = next_arrival_time(tcbs, now);
            if (!nxt.has_value()) break;
            int nxtt = nxt.value();
            if (nxtt > now) gantt.push_back({now, nxtt, string("idle")});
            now = nxtt;
            last_tid = "";
            continue;
        }
        sort(avail.begin(), avail.end(), [&](int a, int b){
            if (tcbs[a].remaining != tcbs[b].remaining) return tcbs[a].remaining < tcbs[b].remaining;
            if (tcbs[a].arrival != tcbs[b].arrival) return tcbs[a].arrival < tcbs[b].arrival;
            return tcbs[a].tid < tcbs[b].tid;
        });
        TCB &cur = tcbs[avail.front()];
        if (cur.start_time == -1) cur.start_time = now;
        int run_until = now + 1; // one tick
        if (!gantt.empty() && gantt.back().end == now && gantt.back().tid == cur.tid) {
            gantt.back().end = run_until;
        } else {
            gantt.push_back({now, run_until, cur.tid});
        }
        cur.remaining -= 1;
        now = run_until;
        if (cur.remaining == 0) cur.completion = now;
        last_tid = cur.tid;
    }
    return {tcbs, gantt};
}

// Priority scheduling preemptive (lower priority value == higher priority)
static pair<vector<TCB>, vector<GanttEntry>> schedule_priority_preemptive(const vector<TCB>& tcbs_in) {
    auto tcbs = clone_tcbs(tcbs_in);
    vector<GanttEntry> gantt;
    int now = 0;
    string last_tid = "";
    while (true) {
        vector<int> avail;
        for (size_t i=0;i<tcbs.size();++i) if (tcbs[i].arrival<=now && tcbs[i].completion==-1 && tcbs[i].remaining>0) avail.push_back(i);
        if (avail.empty()) {
            auto nxt = next_arrival_time(tcbs, now);
            if (!nxt.has_value()) break;
            int nxtt = nxt.value();
            if (nxtt > now) gantt.push_back({now, nxtt, string("idle")});
            now = nxtt;
            last_tid = "";
            continue;
        }
        sort(avail.begin(), avail.end(), [&](int a, int b){
            if (tcbs[a].priority != tcbs[b].priority) return tcbs[a].priority < tcbs[b].priority;
            if (tcbs[a].arrival != tcbs[b].arrival) return tcbs[a].arrival < tcbs[b].arrival;
            return tcbs[a].tid < tcbs[b].tid;
        });
        TCB &cur = tcbs[avail.front()];
        if (cur.start_time == -1) cur.start_time = now;
        int run_until = now + 1;
        if (!gantt.empty() && gantt.back().end == now && gantt.back().tid == cur.tid) {
            gantt.back().end = run_until;
        } else {
            gantt.push_back({now, run_until, cur.tid});
        }
        cur.remaining -= 1;
        now = run_until;
        if (cur.remaining == 0) cur.completion = now;
        last_tid = cur.tid;
    }
    return {tcbs, gantt};
}

// Round Robin
static pair<vector<TCB>, vector<GanttEntry>> schedule_rr(const vector<TCB>& tcbs_in, int quantum) {
    if (quantum <= 0) throw runtime_error("Quantum must be > 0");
    auto tcbs = clone_tcbs(tcbs_in);
    vector<GanttEntry> gantt;
    deque<int> q; // indices in tcbs
    int now = 0;
    size_t n = tcbs.size();
    // helper to enqueue arrivals up to 'now'
    auto enqueue_new = [&](int upto){
        for (size_t i=0;i<n;++i) {
            if (tcbs[i].arrival > upto - (upto-now) && tcbs[i].arrival <= upto && tcbs[i].completion==-1) {
                // ensure not already in queue and not same as currently running
                bool inq=false;
                for (int id: q) if (id==(int)i) { inq=true; break; }
                if (!inq && tcbs[i].remaining>0) q.push_back(i);
            }
        }
    };
    // initially enqueue arrivals at time 0
    for (size_t i=0;i<n;++i) if (tcbs[i].arrival<=0 && tcbs[i].completion==-1) q.push_back(i);
    while (true) {
        if (q.empty()) {
            auto nxt = next_arrival_time(tcbs, now);
            if (!nxt.has_value()) break;
            int nxtt = nxt.value();
            if (nxtt > now) gantt.push_back({now, nxtt, string("idle")});
            now = nxtt;
            // enqueue arrivals that happen at now
            for (size_t i=0;i<n;++i) if (tcbs[i].arrival<=now && tcbs[i].completion==-1) {
                bool inq=false; for (int id: q) if (id==(int)i) { inq=true; break; }
                if (!inq && tcbs[i].remaining>0) q.push_back(i);
            }
            continue;
        }
        int idx = q.front(); q.pop_front();
        TCB &cur = tcbs[idx];
        if (cur.start_time == -1) cur.start_time = now;
        int run = min(quantum, cur.remaining);
        int start = now;
        int endt = now + run;
        // append to gantt (merge contiguous same-tid)
        if (!gantt.empty() && gantt.back().end == start && gantt.back().tid == cur.tid) {
            gantt.back().end = endt;
        } else {
            gantt.push_back({start, endt, cur.tid});
        }
        cur.remaining -= run;
        now = endt;
        // enqueue arrivals that came during (start, endt]
        for (size_t i=0;i<n;++i) {
            if (tcbs[i].arrival>start && tcbs[i].arrival<=endt && tcbs[i].completion==-1) {
                bool inq=false; for (int id: q) if (id==(int)i) { inq=true; break; }
                if (!inq && tcbs[i].remaining>0 && (int)i!=idx) q.push_back(i);
            } else if (tcbs[i].arrival==start && tcbs[i].arrival<=endt && tcbs[i].completion==-1) {
                // already handled when enqueuing prior; safe to skip
            }
        }
        if (cur.remaining>0) {
            q.push_back(idx);
        } else {
            cur.completion = now;
        }
    }
    return {tcbs, gantt};
}

int main(int argc, char** argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    // simple arg parsing
    string algorithm = "FCFS";
    string input_file = "";
    bool preemptive = false;
    int quantum = 2;

    for (int i=1;i<argc;i++) {
        string a = argv[i];
        if (a=="--algo" || a=="-a") { if (i+1<argc) { algorithm = argv[++i]; } }
        else if (a=="--input" || a=="-i") { if (i+1<argc) input_file = argv[++i]; }
        else if (a=="--preemptive" || a=="-p") { preemptive = true; }
        else if (a=="--quantum" || a=="-q") { if (i+1<argc) quantum = stoi(argv[++i]); }
        else if (a=="--help" || a=="-h") {
            cout << "Usage: " << argv[0] << " --algo {FCFS,SJF,PRIORITY,RR} [--preemptive] [--quantum n] [--input file]\n";
            return 0;
        }
    }
    // normalize algorithm name uppercase
    for (auto &c: algorithm) c = toupper((unsigned char)c);

    vector<string> lines;
    if (!input_file.empty()) {
        ifstream ifs(input_file);
        if (!ifs) { cerr << "Cannot open input file: " << input_file << "\n"; return 1; }
        string ln;
        while (getline(ifs, ln)) lines.push_back(ln);
    } else {
        cout << "Enter threads, one per line: ID arrival burst [priority]. Blank line to end.\n";
        string ln;
        while (true) {
            if (!std::getline(cin, ln)) break;
            if (ln.size()==0) break;
            lines.push_back(ln);
        }
    }

    auto tcbs = parse_input_lines(lines);
    if (tcbs.empty()) { cerr << "No threads provided. Exiting.\n"; return 1; }

    pair<vector<TCB>, vector<GanttEntry>> result;
    try {
        if (algorithm=="FCFS") {
            result = schedule_fcfs(tcbs);
        } else if (algorithm=="SJF") {
            if (preemptive) result = schedule_sjf_preemptive(tcbs);
            else result = schedule_sjf_nonpreemptive(tcbs);
        } else if (algorithm=="PRIORITY") {
            // use preemptive priority scheduling
            result = schedule_priority_preemptive(tcbs);
        } else if (algorithm=="RR") {
            result = schedule_rr(tcbs, quantum);
        } else {
            cerr << "Unknown algorithm: " << algorithm << "\n";
            return 1;
        }
    } catch (const exception &ex) {
        cerr << "Error during scheduling: " << ex.what() << "\n";
        return 1;
    }

    cout << "\nAlgorithm: " << algorithm;
    if (algorithm=="SJF") cout << (preemptive ? " (preemptive)" : " (non-preemptive)");
    if (algorithm=="RR") cout << " (quantum=" << quantum << ")";
    cout << "\n";

    print_summary(result.first, result.second);
    return 0;
}
