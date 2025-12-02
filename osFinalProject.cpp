#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <string>
#include <iomanip>

using namespace std;

struct ThreadInfo
{
    string id;
    int arrival;
    int burst;
    int priority;

    int remaining;
    int completion;
    int turnaround;
    int waiting;
};

struct Block
{
    int start;
    int end;
    string id; // can be thread id or "IDLE"
};

void resetThreads(vector<ThreadInfo> &threads)
{
    for (size_t i = 0; i < threads.size(); i++)
    {
        threads[i].remaining = threads[i].burst;
        threads[i].completion = 0;
        threads[i].turnaround = 0;
        threads[i].waiting = 0;
    }
}

void computeMetrics(vector<ThreadInfo> &threads, double &avgWait, double &avgTurn, double &cpuUtil, double &throughput, int totalTime)
{
    int n = (int)threads.size();
    int totalWait = 0;
    int totalTurn = 0;
    int totalBurst = 0;

    for (int i = 0; i < n; i++)
    {
        threads[i].turnaround = threads[i].completion - threads[i].arrival;
        threads[i].waiting = threads[i].turnaround - threads[i].burst;

        totalWait += threads[i].waiting;
        totalTurn += threads[i].turnaround;
        totalBurst += threads[i].burst;
    }

    avgWait = (double)totalWait / n;
    avgTurn = (double)totalTurn / n;

    if (totalTime == 0)
    {
        cpuUtil = 0.0;
        throughput = 0.0;
    }
    else
    {
        cpuUtil = ((double)totalBurst / (double)totalTime) * 100.0;
        throughput = (double)n / (double)totalTime;
    }
}

void printResults(const string &name, vector<ThreadInfo> threads, const vector<Block> &gantt)
{
    double avgWait = 0.0;
    double avgTurn = 0.0;
    double cpuUtil = 0.0;
    double throughput = 0.0;

    int totalTime = 0;
    if (!gantt.empty())
    {
        totalTime = gantt.back().end;
    }

    computeMetrics(threads, avgWait, avgTurn, cpuUtil, throughput, totalTime);

    cout << "\n=============================\n";
    cout << name << " RESULTS\n";
    cout << "=============================\n";

    cout << "\nGantt Chart:\n";
    for (size_t i = 0; i < gantt.size(); i++)
    {
        cout << "[" << gantt[i].start << "-" << gantt[i].end << "] " << gantt[i].id << "  ";
    }
    cout << "\n\n";

    cout << left << setw(8) << "ID"
         << setw(10) << "Arrive"
         << setw(8) << "Burst"
         << setw(10) << "Prior"
         << setw(12) << "Complete"
         << setw(12) << "Waiting"
         << setw(12) << "Turnaround" << "\n";

    for (size_t i = 0; i < threads.size(); i++)
    {
        cout << left << setw(8) << threads[i].id
             << setw(10) << threads[i].arrival
             << setw(8) << threads[i].burst
             << setw(10) << threads[i].priority
             << setw(12) << threads[i].completion
             << setw(12) << threads[i].waiting
             << setw(12) << threads[i].turnaround << "\n";
    }

    cout << "\nAverage waiting time   : " << avgWait << "\n";
    cout << "Average turnaround time: " << avgTurn << "\n";
    cout << "CPU Utilization        : " << cpuUtil << " %\n";
    cout << "Throughput             : " << throughput << " threads/unit time\n";
    cout << "Total simulation time  : " << totalTime << "\n\n";
}

// FCFS (non-preemptive)
void scheduleFCFS(vector<ThreadInfo> threads)
{
    resetThreads(threads);
    vector<Block> gantt;

    // Sort by arrival time (and maybe ID as tie-breaker)
    sort(threads.begin(), threads.end(), [](const ThreadInfo &a, const ThreadInfo &b)
    {
        if (a.arrival == b.arrival)
        {
            return a.id < b.id;
        }
        return a.arrival < b.arrival;
    });

    int time = 0;
    for (size_t i = 0; i < threads.size(); i++)
    {
        if (time < threads[i].arrival)
        {
            Block idleBlock;
            idleBlock.start = time;
            idleBlock.end = threads[i].arrival;
            idleBlock.id = "IDLE";
            gantt.push_back(idleBlock);

            time = threads[i].arrival;
        }

        Block blk;
        blk.start = time;
        blk.end = time + threads[i].burst;
        blk.id = threads[i].id;
        gantt.push_back(blk);

        time += threads[i].burst;
        threads[i].completion = time;
    }

    printResults("FCFS", threads, gantt);
}

// SJF (non-preemptive)
void scheduleSJF(vector<ThreadInfo> threads)
{
    resetThreads(threads);
    vector<Block> gantt;

    int n = (int)threads.size();
    int done = 0;
    int time = 0;

    while (done < n)
    {
        int idx = -1;
        int bestBurst = 1000000000;

        for (int i = 0; i < n; i++)
        {
            if (threads[i].remaining > 0 && threads[i].arrival <= time)
            {
                if (threads[i].burst < bestBurst)
                {
                    bestBurst = threads[i].burst;
                    idx = i;
                }
                else if (threads[i].burst == bestBurst)
                {
                    if (threads[i].arrival < threads[idx].arrival)
                    {
                        idx = i;
                    }
                }
            }
        }

        if (idx == -1)
        {
            int nextArrival = 1000000000;
            for (int i = 0; i < n; i++)
            {
                if (threads[i].remaining > 0)
                {
                    if (threads[i].arrival < nextArrival)
                    {
                        nextArrival = threads[i].arrival;
                    }
                }
            }

            Block idleBlock;
            idleBlock.start = time;
            idleBlock.end = nextArrival;
            idleBlock.id = "IDLE";
            gantt.push_back(idleBlock);

            time = nextArrival;
            continue;
        }

        Block blk;
        blk.start = time;
        blk.end = time + threads[idx].burst;
        blk.id = threads[idx].id;
        gantt.push_back(blk);

        time += threads[idx].burst;
        threads[idx].remaining = 0;
        threads[idx].completion = time;
        done++;
    }

    printResults("SJF (Non-preemptive)", threads, gantt);
}

// Priority (preemptive, smaller priority number = higher priority)
void schedulePriority(vector<ThreadInfo> threads)
{
    resetThreads(threads);
    vector<Block> gantt;

    int n = (int)threads.size();
    int done = 0;
    int time = 0;

    string currentId = "";
    int blockStart = 0;

    while (done < n)
    {
        int idx = -1;
        int bestPrio = 1000000000;

        for (int i = 0; i < n; i++)
        {
            if (threads[i].remaining > 0 && threads[i].arrival <= time)
            {
                if (threads[i].priority < bestPrio)
                {
                    bestPrio = threads[i].priority;
                    idx = i;
                }
                else if (threads[i].priority == bestPrio)
                {
                    if (threads[i].arrival < threads[idx].arrival)
                    {
                        idx = i;
                    }
                }
            }
        }

        if (idx == -1)
        {
            int nextArrival = 1000000000;
            for (int i = 0; i < n; i++)
            {
                if (threads[i].remaining > 0)
                {
                    if (threads[i].arrival < nextArrival)
                    {
                        nextArrival = threads[i].arrival;
                    }
                }
            }

            if (currentId != "")
            {
                Block blk;
                blk.start = blockStart;
                blk.end = time;
                blk.id = currentId;
                gantt.push_back(blk);
                currentId = "";
            }

            Block idleBlock;
            idleBlock.start = time;
            idleBlock.end = nextArrival;
            idleBlock.id = "IDLE";
            gantt.push_back(idleBlock);

            time = nextArrival;
            continue;
        }

        if (currentId == "")
        {
            currentId = threads[idx].id;
            blockStart = time;
        }
        else if (currentId != threads[idx].id)
        {
            Block blk;
            blk.start = blockStart;
            blk.end = time;
            blk.id = currentId;
            gantt.push_back(blk);

            currentId = threads[idx].id;
            blockStart = time;
        }

        time += 1;
        threads[idx].remaining--;

        if (threads[idx].remaining == 0)
        {
            threads[idx].completion = time;
            done++;
        }
    }

    if (currentId != "")
    {
        Block blk;
        blk.start = blockStart;
        blk.end = time;
        blk.id = currentId;
        gantt.push_back(blk);
    }

    printResults("Priority (Preemptive)", threads, gantt);
}

// Round Robin
void scheduleRR(vector<ThreadInfo> threads, int quantum)
{
    resetThreads(threads);
    vector<Block> gantt;

    int n = (int)threads.size();
    int done = 0;
    int time = 0;

    // sort by arrival
    vector<int> order(n);
    for (int i = 0; i < n; i++)
    {
        order[i] = i;
    }

    sort(order.begin(), order.end(), [&](int a, int b)
    {
        if (threads[a].arrival == threads[b].arrival)
        {
            return threads[a].id < threads[b].id;
        }
        return threads[a].arrival < threads[b].arrival;
    });

    queue<int> q;
    int nextIndex = 0;

    while (done < n)
    {
        while (nextIndex < n && threads[order[nextIndex]].arrival <= time)
        {
            q.push(order[nextIndex]);
            nextIndex++;
        }

        if (q.empty())
        {
            if (nextIndex < n)
            {
                int nextArr = threads[order[nextIndex]].arrival;
                Block idleBlock;
                idleBlock.start = time;
                idleBlock.end = nextArr;
                idleBlock.id = "IDLE";
                gantt.push_back(idleBlock);

                time = nextArr;
                while (nextIndex < n && threads[order[nextIndex]].arrival <= time)
                {
                    q.push(order[nextIndex]);
                    nextIndex++;
                }
                continue;
            }
            else
            {
                break;
            }
        }

        int idx = q.front();
        q.pop();

        int runTime = quantum;
        if (threads[idx].remaining < runTime)
        {
            runTime = threads[idx].remaining;
        }

        Block blk;
        blk.start = time;
        blk.end = time + runTime;
        blk.id = threads[idx].id;
        gantt.push_back(blk);

        time += runTime;
        threads[idx].remaining -= runTime;

        while (nextIndex < n && threads[order[nextIndex]].arrival <= time)
        {
            q.push(order[nextIndex]);
            nextIndex++;
        }

        if (threads[idx].remaining > 0)
        {
            q.push(idx);
        }
        else
        {
            threads[idx].completion = time;
            done++;
        }
    }

    printResults("Round Robin", threads, gantt);
}

int main()
{
    int n;
    cout << "How many threads? ";
    cin >> n;

    vector<ThreadInfo> threads(n);

    cout << "Enter threads as: ID Arrival Burst Priority\n";
    cout << "(Smaller priority number = higher priority)\n";

    for (int i = 0; i < n; i++)
    {
        cout << "Thread " << (i + 1) << ": ";
        cin >> threads[i].id >> threads[i].arrival >> threads[i].burst >> threads[i].priority;
        threads[i].remaining = threads[i].burst;
        threads[i].completion = 0;
        threads[i].turnaround = 0;
        threads[i].waiting = 0;
    }

    int quantum;
    cout << "Enter time quantum for Round Robin: ";
    cin >> quantum;

    scheduleFCFS(threads);
    scheduleSJF(threads);
    schedulePriority(threads);
    scheduleRR(threads, quantum);

    return 0;
}
