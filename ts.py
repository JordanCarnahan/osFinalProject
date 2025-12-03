# Thread Scheduling Simulator
# FCFS, SJF (non-preemptive), SJF (preemptive), Priority (preemptive), Round Robin
# Requires: matplotlib

import matplotlib.pyplot as plt
import random

# Read input
def read_threads_from_user():
    print("Choose input mode:")
    print("1 = Manual entry")
    print("2 = Randomly generate threads")
    mode = input("Enter choice: ").strip()

    # Manual Mode
    if mode == "1":
        print("How many threads?")
        n = int(input().strip())

        print("Enter threads as: ID Arrival Burst Priority")
        print("(Smaller priority number = HIGHER priority)")

        threads = []
        for i in range(n):
            print(f"Thread {i+1}: ")
            tid, arr, burst, prio = input().split()
            arr = int(arr)
            burst = int(burst)
            prio = int(prio)
            t = {
                "id": tid,
                "arrival": arr,
                "burst": burst,
                "priority": prio
            }
            threads.append(t)

        print("Enter time quantum for Round Robin: ")
        quantum = int(input().strip())
        return threads, quantum

    # RANDOM MODE
    elif mode == "2":
        print("How many random threads? ")
        n = int(input().strip())

        print("Max arrival time? (0 -> 20): ")
        max_arr = int(input().strip())

        print("Max burst time? (0 -> 10):")
        max_burst = int(input().strip())

        print("Max priority number? (0 -> 5; smaller = higher priority): ")
        max_prio = int(input().strip())

        print("Enter time quantum for Round Robin: ")
        quantum = int(input().strip())

        threads = []
        for i in range(1, n + 1):
            t = {
                "id": f"T{i}",
                "arrival": random.randint(0, max_arr),
                "burst": random.randint(1, max_burst),
                "priority": random.randint(1, max_prio)
            }
            threads.append(t)

        print("\nGenerated Threads:")
        print("ID  Arr  Burst  Priority")
        for t in threads:
            print(f"{t['id']:3} {t['arrival']:3} {t['burst']:5} {t['priority']:8}")

        return threads, quantum

    else:
        print("Invalid choice. Try again.")
        return read_threads_from_user()

# Common metric printing
def print_metrics(name, result_threads):
    print("\n==============================")
    print(f"{name} RESULTS")
    print("==============================")
    print("ID  Arr  Burst  Prio  Comp  Wait  Turn")

    total_wait = 0
    total_turn = 0
    first_arr = min(t["arrival"] for t in result_threads)
    last_comp = max(t["completion"] for t in result_threads)
    total_burst = sum(t["burst"] for t in result_threads)

    for t in result_threads:
        total_wait += t["waiting"]
        total_turn += t["turnaround"]
        print(
            f"{t['id']:>2}  {t['arrival']:>3}  {t['burst']:>5}  {t['priority']:>4}  "
            f"{t['completion']:>4}  {t['waiting']:>4}  {t['turnaround']:>4}"
        )

    n = len(result_threads)
    avg_wait = total_wait / n
    avg_turn = total_turn / n

    total_time = last_comp - first_arr
    if total_time == 0:
        cpu_util = 100.0
        throughput = 0
    else:
        cpu_util = (total_burst / total_time) * 100.0
        throughput = n / total_time

    print(f"\nAverage waiting time     = {avg_wait:.2f}")
    print(f"Average turnaround time  = {avg_turn:.2f}")
    print(f"CPU utilization          = {cpu_util:.2f}%")
    print(f"Throughput (threads/time)= {throughput:.2f}")

# Helper: Gantt chart plotting
# schedule is list of (id, start, end)
def draw_gantt(schedule, title):
    # Collect unique IDs except 'IDLE'
    ids = []
    for tid, s, e in schedule:
        if tid != "IDLE" and tid not in ids:
            ids.append(tid)

    # Put IDLE at the bottom if it exists
    has_idle = any(tid == "IDLE" for tid, _, _ in schedule)
    if has_idle:
        ids.insert(0, "IDLE")

    # y positions for each ID
    y_pos = {}
    for i, tid in enumerate(ids):
        y_pos[tid] = i

    fig, ax = plt.subplots()
    for tid, start, end in schedule:
        y = y_pos[tid]
        # Draw a horizontal bar segment for each run slice
        ax.barh(y, end - start, left=start, edgecolor="black")
        ax.text((start + end) / 2, y, tid, va="center", ha="center", fontsize=8)

    ax.set_yticks(list(y_pos.values()))
    ax.set_yticklabels(ids)
    ax.set_xlabel("Time")
    ax.set_title(title + " - Gantt Chart")
    plt.tight_layout()
    plt.show()

# FCFS (non-preemptive)
def run_fcfs(threads):
    # Make a copy of the input
    ts = []
    for t in threads:
        ts.append({
            "id": t["id"],
            "arrival": t["arrival"],
            "burst": t["burst"],
            "priority": t["priority"],
            "remaining": t["burst"],
            "completion": 0,
            "waiting": 0,
            "turnaround": 0
        })

    # Sort by arrival time
    ts.sort(key=lambda x: x["arrival"])

    schedule = []
    time = 0

    for t in ts:
        if time < t["arrival"]:
            # CPU idle gap
            schedule.append(("IDLE", time, t["arrival"]))
            time = t["arrival"]

        start = time
        end = time + t["burst"]
        schedule.append((t["id"], start, end))
        time = end

        t["completion"] = time
        t["turnaround"] = t["completion"] - t["arrival"]
        t["waiting"] = t["turnaround"] - t["burst"]

    return ts, schedule

# SJF (non-preemptive)
def run_sjf(threads):
    ts = []
    for t in threads:
        ts.append({
            "id": t["id"],
            "arrival": t["arrival"],
            "burst": t["burst"],
            "priority": t["priority"],
            "remaining": t["burst"],
            "completion": 0,
            "waiting": 0,
            "turnaround": 0
        })

    schedule = []
    time = 0
    finished = 0
    n = len(ts)

    while finished < n:
        # Get all threads that have arrived and are not done
        ready = []
        for t in ts:
            if t["arrival"] <= time and t["remaining"] > 0:
                ready.append(t)

        if not ready:
            # No ready threads: jump to next arrival
            next_arrival = min(t["arrival"] for t in ts if t["remaining"] > 0)
            schedule.append(("IDLE", time, next_arrival))
            time = next_arrival
            continue

        # Pick shortest job (by burst)
        ready.sort(key=lambda x: x["burst"])
        t = ready[0]

        start = time
        end = time + t["burst"]
        schedule.append((t["id"], start, end))
        time = end

        t["remaining"] = 0
        t["completion"] = time
        t["turnaround"] = t["completion"] - t["arrival"]
        t["waiting"] = t["turnaround"] - t["burst"]
        finished += 1

    return ts, schedule

# SJF (preemptive) = SRTF
# run in 1-time-unit steps
def run_sjf_preemptive(threads):
    ts = []
    for t in threads:
        ts.append({
            "id": t["id"],
            "arrival": t["arrival"],
            "burst": t["burst"],
            "priority": t["priority"],
            "remaining": t["burst"],
            "completion": 0,
            "waiting": 0,
            "turnaround": 0
        })

    schedule = []
    time = 0
    finished = 0
    n = len(ts)

    while finished < n:
        # Threads that have arrived and still need CPU
        ready = []
        for t in ts:
            if t["arrival"] <= time and t["remaining"] > 0:
                ready.append(t)

        if not ready:
            # If no one is ready, CPU is idle until the next arrival
            next_arrival = min(t["arrival"] for t in ts if t["remaining"] > 0)
            schedule.append(("IDLE", time, next_arrival))
            time = next_arrival
            continue

        # Pick the thread with the shortest remaining time
        ready.sort(key=lambda x: x["remaining"])
        t = ready[0]

        # Run it for 1 time unit, then re-check
        start = time
        end = time + 1
        schedule.append((t["id"], start, end))
        time = end
        t["remaining"] -= 1

        if t["remaining"] == 0:
            t["completion"] = time
            t["turnaround"] = t["completion"] - t["arrival"]
            t["waiting"] = t["turnaround"] - t["burst"]
            finished += 1

    return ts, schedule

# Priority (preemptive, smaller = higher)
# simple 1-time-unit steps

def run_priority_preemptive(threads):
    ts = []
    for t in threads:
        ts.append({
            "id": t["id"],
            "arrival": t["arrival"],
            "burst": t["burst"],
            "priority": t["priority"],
            "remaining": t["burst"],
            "completion": 0,
            "waiting": 0,
            "turnaround": 0
        })

    schedule = []
    time = 0
    n = len(ts)
    finished = 0

    while finished < n:
        # All arrived and not finished
        ready = []
        for t in ts:
            if t["arrival"] <= time and t["remaining"] > 0:
                ready.append(t)

        if not ready:
            # idle until next arrival
            next_arrival = min(t["arrival"] for t in ts if t["remaining"] > 0)
            schedule.append(("IDLE", time, next_arrival))
            time = next_arrival
            continue

        # pick highest priority (smallest number)
        ready.sort(key=lambda x: x["priority"])
        t = ready[0]

        # run for 1 time unit (preemptive)
        start = time
        end = time + 1
        schedule.append((t["id"], start, end))
        time = end
        t["remaining"] -= 1

        if t["remaining"] == 0:
            t["completion"] = time
            t["turnaround"] = t["completion"] - t["arrival"]
            t["waiting"] = t["turnaround"] - t["burst"]
            finished += 1

    return ts, schedule

# Round Robin
def run_round_robin(threads, quantum):
    ts = []
    for t in threads:
        ts.append({
            "id": t["id"],
            "arrival": t["arrival"],
            "burst": t["burst"],
            "priority": t["priority"],
            "remaining": t["burst"],
            "completion": 0,
            "waiting": 0,
            "turnaround": 0
        })

    schedule = []
    time = 0
    n = len(ts)
    finished = 0

    # ready queue stores indices into ts
    ready = []

    # Sort indices by arrival so we can bring them in order
    ts_sorted = sorted(range(n), key=lambda i: ts[i]["arrival"])
    next_index_pos = 0

    while finished < n:
        # Add newly arrived threads to ready queue
        while (next_index_pos < n and
               ts[ts_sorted[next_index_pos]]["arrival"] <= time):
            ready.append(ts_sorted[next_index_pos])
            next_index_pos += 1

        if not ready:
            # CPU idle, jump to next arrival
            if next_index_pos < n:
                next_arrival = ts[ts_sorted[next_index_pos]]["arrival"]
                schedule.append(("IDLE", time, next_arrival))
                time = next_arrival
                continue
            else:
                # no one left
                break

        # Pop first from queue
        idx = ready.pop(0)
        t = ts[idx]

        # Set quantum to slice time
        slice_time = quantum
        if t["remaining"] < quantum:
            slice_time = t["remaining"]

        start = time
        end = time + slice_time
        schedule.append((t["id"], start, end))
        time = end
        t["remaining"] -= slice_time

        # Add any new arrivals that came during this slice
        while (next_index_pos < n and
               ts[ts_sorted[next_index_pos]]["arrival"] <= time):
            ready.append(ts_sorted[next_index_pos])
            next_index_pos += 1

        if t["remaining"] > 0:
            # still needs CPU, push back into queue
            ready.append(idx)
        else:
            # done
            t["completion"] = time
            t["turnaround"] = t["completion"] - t["arrival"]
            t["waiting"] = t["turnaround"] - t["burst"]
            finished += 1

    return ts, schedule

# Main
def main():
    # 1) Input
    threads, quantum = read_threads_from_user()

    # 2) Run all algorithms
    fcfs_threads, fcfs_sched = run_fcfs(threads)
    sjf_np_threads, sjf_np_sched = run_sjf(threads)
    sjf_p_threads, sjf_p_sched = run_sjf_preemptive(threads)
    prio_threads, prio_sched = run_priority_preemptive(threads)
    rr_threads, rr_sched = run_round_robin(threads, quantum)

    # 3) Print metrics
    print_metrics("FCFS", fcfs_threads)
    print_metrics("SJF (non-preemptive)", sjf_np_threads)
    print_metrics("SJF (preemptive)", sjf_p_threads)
    print_metrics("Priority (preemptive)", prio_threads)
    print_metrics("Round Robin", rr_threads)

    # 4) Draw Gantt charts
    draw_gantt(fcfs_sched, "FCFS")
    draw_gantt(sjf_np_sched, "SJF (non-preemptive)")
    draw_gantt(sjf_p_sched, "SJF (preemptive)")
    draw_gantt(prio_sched, "Priority (Preemptive)")
    draw_gantt(rr_sched, "Round Robin")

if __name__ == "__main__":
    main()
