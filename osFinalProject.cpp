// osFinalProject.cpp
// Simple Thread Scheduling Simulator
// Allman-style braces version

#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <string>
#include <iomanip>
using namespace std;

struct ThreadTCB
{
    string id;
    int arrival;
    int burst;
    int priority;
    int remaining;
    int completion;
};

struct GanttEntry
{
    string id;
    int start;
    int end;
};

void printGantt(const string &name, const vector<GanttEntry> &gantt)
{
    cout << "\n=== " << name << " Gantt Chart ===\n";

    if (gantt.empty())
    {
        cout << "No execution recorded.\n";
        return;
    }

    for (size_t i = 0; i < gantt.size(); i++)
    {
        cout << "[" << gantt[i].start << ", " << gantt[i].end << ") "
             << gantt[i].id << "\n";
    }
}

void printMetrics(const string &name, const vector<ThreadTCB> &threads,
                  int totalBusyTime, int startTime, int endTime)
{
    cout << "\n=== " << name << " Metrics ===\n";

    int n = threads.size();
    double sumWaiting = 0;
    double sumTurnaround = 0;

    cout << "Thread\tArr\tBurst\tCompl\tWait\tTurn\n";

    for (int i = 0; i < n; i++)
    {
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

    if (endTime <= startTime)
    {
        endTime = startTime + 1;
    }

    double avgWaiting = sumWaiting / n;
    double avgTurnaround = sumTurnaround / n;
    double cpuUtil = (double)totalBusyTime / (endTime - startTime) * 100.0;
    double throughput = (double)n / (endTime - startTime);

    cout << fixed << setprecision(2);
    cout << "\nAverage waiting time: " << avgWaiting << "\n";
    cout << "Average turnaround time: " << avgTurnaround << "\n";
    cout << "CPU utilization: " << cpuUtil << " %\n";
    cout << "Throughput: " << throughput << " threads/time unit\n";
}

void simulateFCFS(vector<ThreadTCB> threads)
{
    sort(threads.begin(), threads.end(),
         [](const ThreadTCB &a, const ThreadTCB &b)
         {
             return a.arrival < b.arrival;
         });

    int n = threads.size();
    int currentTime = 0;
    int totalBusyTime = 0;
    vector<GanttEntry> gantt;

    int minArrival = threads[0].arrival;

    for (auto &t : threads)
    {
        if (t.arrival < minArrival)
        {
            minArrival = t.arrival;
        }
    }

    for (int i = 0; i < n; i++)
    {
        if (currentTime < threads[i].arrival)
        {
            gantt.push_back({"IDLE", currentTime, threads[i].arrival});
            currentTime = threads[i].arrival;
        }

        gantt.push_back({threads[i].id, currentTime, currentTime + threads[i].burst});

        currentTime += threads[i].burst;
        threads[i].completion = currentTime;
        totalBusyTime += threads[i].burst;
    }

    printGantt("FCFS", gantt);
    printMetrics("FCFS", threads, totalBusyTime, minArrival, currentTime);
}

void simulateSJF(vector<ThreadTCB> threads)
{
    int n = threads.size();

    for (int i = 0; i < n; i++)
    {
        threads[i].remaining = threads[i].burst;
        threads[i].completion = -1;
    }

    int currentTime = 0;
    int completed = 0;
    int totalBusyTime = 0;
    vector<GanttEntry> gantt;
    vector<bool> done(n, false);

    int minArrival = threads[0].arrival;

    for (auto &t : threads)
    {
        if (t.arrival < minArrival)
        {
            minArrival = t.arrival;
        }
    }

    currentTime = minArrival;

    while (completed < n)
    {
        int idx = -1;
        int minBurst = 1e9;

        for (int i = 0; i < n; i++)
        {
            if (!done[i] && threads[i].arrival <= currentTime)
            {
                if (threads[i].burst < minBurst)
                {
                    minBurst = threads[i].burst;
                    idx = i;
                }
            }
        }

        if (idx == -1)
        {
            gantt.push_back({"IDLE", currentTime, currentTime + 1});
            currentTime++;
            continue;
        }

        gantt.push_back({threads[idx].id, currentTime, currentTime + threads[idx].burst});
        currentTime += threads[idx].burst;

        threads[idx].completion = currentTime;
        done[idx] = true;
        completed++;
        totalBusyTime += threads[idx].burst;
    }

    printGantt("SJF", gantt);
    printMetrics("SJF", threads, totalBusyTime, minArrival, currentTime);
}

void simulatePriority(vector<ThreadTCB> threads)
{
    int n = threads.size();

    for (auto &t : threads)
    {
        t.remaining = t.burst;
        t.completion = -1;
    }

    int currentTime = 0;
    int completed = 0;
    int totalBusyTime = 0;
    vector<GanttEntry> gantt;
    vector<bool> done(n, false);

    int minArrival = threads[0].arrival;

    for (auto &t : threads)
    {
        if (t.arrival < minArrival)
        {
            minArrival = t.arrival;
        }
    }

    currentTime = minArrival;

    while (completed < n)
    {
        int idx = -1;
        int bestP = 1e9;

        for (int i = 0; i < n; i++)
        {
            if (!done[i] && threads[i].arrival <= currentTime)
            {
                if (threads[i].priority < bestP)
                {
                    bestP = threads[i].priority;
                    idx = i;
                }
            }
        }

        if (idx == -1)
        {
            gantt.push_back({"IDLE", currentTime, currentTime + 1});
            currentTime++;
            continue;
        }

        gantt.push_back({threads[idx].id, currentTime, currentTime + threads[idx].burst});
        currentTime += threads[idx].burst;

        threads[idx].completion = currentTime;
        done[idx] = true;
        completed++;
        totalBusyTime += threads[idx].burst;
    }

    printGantt("Priority", gantt);
    printMetrics("Priority", threads, totalBusyTime, minArrival, currentTime);
}

void simulateRR(vector<ThreadTCB> threads, int quantum)
{
    if (quantum <= 0)
    {
        quantum = 1;
    }

    sort(threads.begin(), threads.end(),
         [](const ThreadTCB &a, const ThreadTCB &b)
         {
             return a.arrival < b.arrival;
         });

    int n = threads.size();

    for (auto &t : threads)
    {
        t.remaining = t.burst;
        t.completion = -1;
    }

    int currentTime = threads[0].arrival;
    int totalBusyTime = 0;
    int completed = 0;
    vector<GanttEntry> gantt;

    queue<int> q;
    vector<bool> added(n, false);

    auto addArrivals = [&](int time)
    {
        for (int i = 0; i < n; i++)
        {
            if (!added[i] && threads[i].arrival <= time)
            {
                q.push(i);
                added[i] = true;
            }
        }
    };

    addArrivals(currentTime);

    while (completed < n)
    {
        if (q.empty())
        {
            gantt.push_back({"IDLE", currentTime, currentTime + 1});
            currentTime++;
            addArrivals(currentTime);
            continue;
        }

        int idx = q.front();
        q.pop();

        int runTime = min(threads[idx].remaining, quantum);

        gantt.push_back({threads[idx].id, currentTime, currentTime + runTime});

        threads[idx].remaining -= runTime;
        currentTime += runTime;
        totalBusyTime += runTime;

        addArrivals(currentTime);

        if (threads[idx].remaining > 0)
        {
            q.push(idx);
        }
        else
        {
            threads[idx].completion = currentTime;
            completed++;
        }
    }

    int minArrival = threads[0].arrival;

    printGantt("Round Robin", gantt);
    printMetrics("Round Robin", threads, totalBusyTime, minArrival, currentTime);
}

int main()
{
    int n;

    cout << "How many threads? ";
    cin >> n;

    if (n <= 0)
    {
        cout << "Invalid number.\n";
        return 0;
    }

    vector<ThreadTCB> threads(n);

    cout << "Enter ID Arrival Burst Priority:\n";

    for (int i = 0; i < n; i++)
    {
        cin >> threads[i].id >> threads[i].arrival >>
               threads[i].burst >> threads[i].priority;

        threads[i].remaining = threads[i].burst;
        threads[i].completion = -1;
    }

    int quantum;
    cout << "Enter Round Robin quantum: ";
    cin >> quantum;

    simulateFCFS(threads);
    simulateSJF(threads);
    simulatePriority(threads);
    simulateRR(threads, quantum);

    return 0;
}