#include <iostream>
#include <vector>
#include <queue>
#include <string>
#include <climits>
#include <iomanip>

using namespace std;

// Structure to hold all details of a process
struct Process {
    string id;
    int arrivalTime;
    int burstTime;
    int remainingTime;
    int currentQueue;     // Tracks which queue the process is currently in

    int completionTime;
    int firstRunTime;     // -1 = not yet run

    Process(string id, int arrival, int burst)
        : id(id), arrivalTime(arrival), burstTime(burst),
          remainingTime(burst), currentQueue(0),
          completionTime(0), firstRunTime(-1) {}
};


int main() {
    int numQueues;

    // 1. Accept Queue Configuration
    cout << "Enter the number of queues: \n";
    cin >> numQueues;

    if (numQueues <= 0) {
        cout << "Error: number of queues must be > 0.\n";
        return 1;
    }

    vector<int> quantums(numQueues);
    for (int i = 0; i < numQueues; ++i) {
        if (i == numQueues - 1) {
            cout << "Enter time quantum for Queue " << i << " (Enter -1 for FCFS): \n";
        } else {
            cout << "Enter time quantum for Queue " << i << ": \n";
        }
        cin >> quantums[i];

        bool isFCFS = (i == numQueues - 1) && (quantums[i] == -1);
        if (!isFCFS && quantums[i] <= 0) {
            cout << "Error: quantum must be > 0 (or -1 for FCFS on the last queue).\n";
            --i;
        }
    }

    // 2. Accept Process Details
    int numProcesses;
    cout << "Enter the number of processes: \n";
    cin >> numProcesses;

    if (numProcesses <= 0) {
        cout << "Error: number of processes must be > 0.\n";
        return 1;
    }

    vector<Process> processes;
    for (int i = 0; i < numProcesses; ++i) {
        string id;
        int arrival, burst;
        cout << "Enter ID, Arrival Time, and Burst Time for Process " << (i + 1) << " (separated by spaces): \n";
        cin >> id >> arrival >> burst;

        if (arrival < 0) {
            cout << "Error: arrival time must be >= 0. Re-enter this process.\n";
            --i; continue;
        }
        if (burst <= 0) {
            cout << "Error: burst time must be > 0. Re-enter this process.\n";
            --i; continue;
        }
        processes.push_back(Process(id, arrival, burst));
    }

    // 3. Initialize MLFQ Data Structures
    vector<queue<Process*>> mlfq(numQueues);
    vector<pair<int, string>> executionSchedule;

    int time = 0;
    int completedProcesses = 0;
    Process* runningProcess = nullptr;
    int timeInQuantum = 0;

    int maxTime = 0;
    for (auto& p : processes) maxTime += p.arrivalTime + p.burstTime;

    cout << "\nStarting MLFQ Simulation...\n";

    // 4. Main Event Loop
    while (completedProcesses < numProcesses && time <= maxTime) {

        // Step A: Handle New Arrivals
        for (auto& p : processes) {
            if (p.arrivalTime == time) {
                mlfq[0].push(&p);
            }
        }

        // Step B (was C): Check for strict preemption by a higher-priority queue
        if (runningProcess != nullptr) {
            for (int i = 0; i < runningProcess->currentQueue; ++i) {
                if (!mlfq[i].empty()) {
                    // Preempt: push running process back to its current queue
                    mlfq[runningProcess->currentQueue].push(runningProcess);
                    runningProcess = nullptr;
                    break;
                }
            }
        }

        // Step C (was B): Check current running process status
        if (runningProcess != nullptr) {
            if (runningProcess->remainingTime == 0) {
                // Process finished
                runningProcess->completionTime = time;
                completedProcesses++;
                runningProcess = nullptr;
            }
            else if (quantums[runningProcess->currentQueue] != -1 &&
                     timeInQuantum == quantums[runningProcess->currentQueue]) {
                // Time quantum exhausted: demote to lower-priority queue
                if (runningProcess->currentQueue < numQueues - 1) {
                    runningProcess->currentQueue++;
                }
                mlfq[runningProcess->currentQueue].push(runningProcess);
                runningProcess = nullptr;
            }
        }

        // Step D: Schedule the next process
        if (runningProcess == nullptr) {
            for (int i = 0; i < numQueues; ++i) {
                if (!mlfq[i].empty()) {
                    runningProcess = mlfq[i].front();
                    mlfq[i].pop();
                    timeInQuantum = 0;  // Fresh quantum on every new scheduling
                    break;
                }
            }
        }

        // Step E: Execute the process for 1 tick
        if (runningProcess != nullptr) {
            if (runningProcess->firstRunTime == -1) {
                runningProcess->firstRunTime = time;
            }
            executionSchedule.push_back({time, runningProcess->id});
            runningProcess->remainingTime--;
            timeInQuantum++;

            // Mark completion if this was the last tick
            if (runningProcess->remainingTime == 0) {
                runningProcess->completionTime = time + 1;
                completedProcesses++;
                runningProcess = nullptr;
            }
        } else {
            executionSchedule.push_back({time, "Idle"});
        }

        time++;
    }

    if (completedProcesses < numProcesses) {
        cout << "\nWarning: simulation halted at time " << time
             << " (not all processes completed — check arrival times).\n";
    }

    // 5. Output: raw execution log
    cout << "\nTime\tCPU Running\n";
    cout << "-------------------\n";
    for (const auto& entry : executionSchedule) {
        cout << entry.first << "\t" << entry.second << "\n";
    }

    // Performance metrics
    cout << "\nPerformance Metrics:\n";
    cout << left
         << setw(8)  << "ID"
         << setw(12) << "Arrival"
         << setw(10) << "Burst"
         << setw(14) << "Completion"
         << setw(14) << "Turnaround"
         << setw(12) << "Waiting"
         << setw(12) << "Response"
         << "\n";
    cout << string(82, '-') << "\n";

    double sumTA = 0, sumWT = 0, sumRT = 0;
    for (auto& p : processes) {
        int ta = p.completionTime - p.arrivalTime;
        int wt = ta - p.burstTime;
        int rt = (p.firstRunTime != -1) ? (p.firstRunTime - p.arrivalTime) : -1;

        sumTA += ta;
        sumWT += wt;
        if (rt >= 0) sumRT += rt;

        cout << left
             << setw(8)  << p.id
             << setw(12) << p.arrivalTime
             << setw(10) << p.burstTime
             << setw(14) << p.completionTime
             << setw(14) << ta
             << setw(12) << wt
             << setw(12) << (rt >= 0 ? to_string(rt) : "N/A")
             << "\n";
    }

    int n = (int)processes.size();
    cout << string(82, '-') << "\n";
    cout << fixed << setprecision(2);
    cout << "Average Turnaround Time : " << sumTA / n << "\n";
    cout << "Average Waiting Time    : " << sumWT / n << "\n";
    cout << "Average Response Time   : " << sumRT / n << "\n";

    return 0;
}
