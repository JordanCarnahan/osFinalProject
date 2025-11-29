// osFinalProject.cpp
// Simple Thread Scheduling Simulator
// CSCI 311 - Final Project (example implementation)

#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <string>
#include <iomanip>

using namespace std;

// Thread Control Block (TCB) simulation
struct ThreadTCB {
    string id;
    int arrival;
    int burst;
    int priority;

    // fields used during simulation
    int remaining;
    int completion;
};

// For Gantt chart entries
struct GanttEntry {
    string id;   // thread id, or "IDLE"
    int start;
    int end;
};

// Helper: print Gantt chart
void printGantt(const string &name, const vector<GanttEntry> &gantt) {
    cout << "\n=== " << name << " Gantt Chart ===\n";
    if (gantt.empty()) {
        cout << "No execution recorded.\n";
        return;
    }
    for (size_t i = 0; i < gantt.size(); i++) {
        cout << "[" << gantt[i].start << ", " << gantt[i].end << ") "
             << gantt[i].id << "\n";
    }
}

// Helper: compute and print metrics
void printMetrics(const string &name,
                  const vector<ThreadTCB> &threads,
                  int totalBusyTime,
                  int startTime,
                  int endTime)
{
    cout << "\n=== " << name << " Metrics ===\n";

    int n = (int)threads.size();
    double sumWaiting = 0.0;
    double sumTurnaround = 0.0;

    cout << "Thread\tArr\tBurst\tCompl\tWait\tTurn\n";
    for (int i = 0; i < n; i++) {
        int turnaround = threads[i].completion - threads[i].arrival;
        int waiting = turnaround - threads[i].burst;

        sumWaiting += waiting;
        sumTurnaround += turnaround;

        cout << threads[i].id << "\t"
             << threads[i].arrival << "\t"
             << threads[i].burst << "\t"
             << threads[i].completion << "\t"
             << waiting << "\t"
             << turnaround << "\n";
    }

    if (endTime <= startTime) {
        endTime = startTime + 1; // avoid divide by zero
    }

    double avgWaiting = sumWaiting / n;
    double avgTurnaround = sumTurnaround / n;
    int totalTime = endTime - startTime;
    double cpuUtil = (double)totalBusyTime / totalTime * 100.0;
    double throughput = (double)n / totalTime;

    cout << fixed << setprecision(2);
    cout << "\nAverage waiting time: " << avgWaiting << "\n";
    cout << "Average turnaround time: " << avgTurnaround << "\n";
    cout << "CPU utilization: " << cpuUtil << " %\n";
    cout << "Throughput: " << throughput << " threads per time unit\n";
}

// FCFS scheduling
void simulateFCFS(vector<ThreadTCB> threads) {
    // Sort by arrival time
    sort(threads.begin(), threads.end(),
         [](const ThreadTCB &a, const ThreadTCB &b) {
             return a.arrival < b.arrival;
         });

    int n = (int)threads.size();
    int currentTime = 0;
    int totalBusyTime = 0;
    vector<GanttEntry> gantt;

    int minArrival = threads[0].arrival;
    for (int i = 0; i < n; i++) {
        if (threads[i].arrival < minArrival) minArrival = threads[i].arrival;
    }

    for (int i = 0; i < n; i++) {
        if (currentTime < threads[i].arrival) {
            // CPU idle
            gantt.push_back({"IDLE", currentTime, threads[i].arrival});
            currentTime = threads[i].arrival;
        }
        int start = currentTime;
        int end = currentTime + threads[i].burst;
        gantt.push_back({threads[i].id, start, end});

        currentTime = end;
        threads[i].completion = end;
        totalBusyTime += threads[i].burst;
    }

    printGantt("FCFS", gantt);
    printMetrics("FCFS", threads, totalBusyTime, minArrival, currentTime);
}

// Shortest Job First (non-preemptive)
void simulateSJF(vector<ThreadTCB> threads) {
    int n = (int)threads.size();
    // initialize fields
    for (int i = 0; i < n; i++) {
        threads[i].remaining = threads[i].burst;
        threads[i].completion = -1;
    }

    int currentTime = 0;
    int completed = 0;
    int totalBusyTime = 0;
    vector<GanttEntry> gantt;

    int minArrival = threads[0].arrival;
    for (int i = 0; i < n; i++) {
        if (threads[i].arrival < minArrival) minArrival = threads[i].arrival;
    }
    currentTime = minArrival;

    vector<bool> done(n, false);

    while (completed < n) {
        int idx = -1;
        int minBurst = 1000000000;

        // pick shortest job among arrived and not done
        for (int i = 0; i < n; i++) {
            if (!done[i] && threads[i].arrival <= currentTime) {
                if (threads[i].burst < minBurst) {
                    minBurst = threads[i].burst;
                    idx = i;
                }
            }
        }

        if (idx == -1) {
            // no job ready, CPU idle
            gantt.push_back({"IDLE", currentTime, currentTime + 1});
            currentTime++;
            continue;
        }

        int start = currentTime;
        int end = currentTime + threads[idx].burst;

        gantt.push_back({threads[idx].id, start, end});
        currentTime = end;
        threads[idx].completion = end;
        totalBusyTime += threads[idx].burst;
        done[idx] = true;
        completed++;
    }

    printGantt("SJF (non-preemptive)", gantt);
    printMetrics("SJF (non-preemptive)", threads, totalBusyTime, minArrival, currentTime);
}

// Priority Scheduling (non-preemptive, smaller priority = higher)
void simulatePriority(vector<ThreadTCB> threads) {
    int n = (int)threads.size();
    for (int i = 0; i < n; i++) {
        threads[i].remaining = threads[i].burst;
        threads[i].completion = -1;
    }

    int currentTime = 0;
    int completed = 0;
    int totalBusyTime = 0;
    vector<GanttEntry> gantt;

    int minArrival = threads[0].arrival;
    for (int i = 0; i < n; i++) {
        if (threads[i].arrival < minArrival) minArrival = threads[i].arrival;
    }
    currentTime = minArrival;

    vector<bool> done(n, false);

    while (completed < n) {
        int idx = -1;
        int bestPriority = 1000000000;

        // pick highest priority (smallest number) among arrived
        for (int i = 0; i < n; i++) {
            if (!done[i] && threads[i].arrival <= currentTime) {
                if (threads[i].priority < bestPriority) {
                    bestPriority = threads[i].priority;
                    idx = i;
                }
            }
        }

        if (idx == -1) {
            // no ready thread
            gantt.push_back({"IDLE", currentTime, currentTime + 1});
            currentTime++;
            continue;
        }

        int start = currentTime;
        int end = currentTime + threads[idx].burst;
        gantt.push_back({threads[idx].id, start, end});

        currentTime = end;
        threads[idx].completion = end;
        totalBusyTime += threads[idx].burst;
        done[idx] = true;
        completed++;
    }

    printGantt("Priority (non-preemptive)", gantt);
    printMetrics("Priority (non-preemptive)", threads, totalBusyTime, minArrival, currentTime);
}

// Round Robin Scheduling
void simulateRR(vector<ThreadTCB> threads, int quantum) {
    int n = (int)threads.size();
    if (quantum <= 0) quantum = 1; // avoid weird stuff

    // sort by arrival so we can process arrivals in order
    sort(threads.begin(), threads.end(),
         [](const ThreadTCB &a, const ThreadTCB &b) {
             return a.arrival < b.arrival;
         });

    for (int i = 0; i < n; i++) {
        threads[i].remaining = threads[i].burst;
        threads[i].completion = -1;
    }

    int currentTime = 0;
    int totalBusyTime = 0;
    vector<GanttEntry> gantt;

    int minArrival = threads[0].arrival;
    for (int i = 0; i < n; i++) {
        if (threads[i].arrival < minArrival) minArrival = threads[i].arrival;
    }
    currentTime = minArrival;

    queue<int> q;
    vector<bool> added(n, false);
    int completed = 0;

    // helper lambda to add newly arrived threads to ready queue
    auto addArrivals = [&](int time) {
        for (int i = 0; i < n; i++) {
            if (!added[i] && threads[i].arrival <= time) {
                q.push(i);
                added[i] = true;
            }
        }
    };

    addArrivals(currentTime);

    while (completed < n) {
        if (q.empty()) {
            // idle until next arrival
            gantt.push_back({"IDLE", currentTime, currentTime + 1});
            currentTime++;
            addArrivals(currentTime);
            continue;
        }

        int idx = q.front();
        q.pop();

        int runTime = (threads[idx].remaining < quantum) ?
                      threads[idx].remaining : quantum;

        int start = currentTime;
        int end = currentTime + runTime;

        gantt.push_back({threads[idx].id, start, end});

        threads[idx].remaining -= runTime;
        currentTime = end;
        totalBusyTime += runTime;

        // add any new arrivals that came during this time slice
        addArrivals(currentTime);

        if (threads[idx].remaining > 0) {
            // not finished yet, put back in queue
            q.push(idx);
        } else {
            threads[idx].completion = currentTime;
            completed++;
        }
    }

    printGantt("Round Robin (q = " + to_string(quantum) + ")", gantt);
    printMetrics("Round Robin", threads, totalBusyTime, minArrival, currentTime);
}

int main() {
    int n;
    cout << "Thread Scheduling Simulator\n";
    cout << "How many threads? ";
    cin >> n;

    if (!cin || n <= 0) {
        cout << "Invalid number of threads.\n";
        return 0;
    }

    vector<ThreadTCB> threads(n);

    cout << "Enter threads as: ID Arrival Burst Priority\n";
    cout << "(Smaller priority number = higher priority)\n";

    for (int i = 0; i < n; i++) {
        cout << "Thread " << (i + 1) << ": ";
        cin >> threads[i].id >> threads[i].arrival
            >> threads[i].burst >> threads[i].priority;
        threads[i].remaining = threads[i].burst;
        threads[i].completion = -1;
    }

    int quantum;
    cout << "Enter time quantum for Round Robin: ";
    cin >> quantum;

    // Run all algorithms on the same input (copies)
    simulateFCFS(threads);
    simulateSJF(threads);
    simulatePriority(threads);
    simulateRR(threads, quantum);

    cout << "\nSimulation complete.\n";
    return 0;
}
