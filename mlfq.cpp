#include <iostream>
#include <vector>
#include <queue>
#include <string>
#include <iomanip>

using namespace std;

struct Process {
    string id;
    int arrivalTime;
    int burstTime;
    int remainingTime;
    int currentQueue;
    int completionTime;
    int firstRunTime;

    Process(string id, int arrival, int burst)
        : id(id), arrivalTime(arrival), burstTime(burst),
          remainingTime(burst), currentQueue(0),
          completionTime(0), firstRunTime(-1) {}
};

int main() {
    int numQueues;
    cout << "Enter the number of queues: ";
    cin >> numQueues;

    vector<int> quantums(numQueues);
    for (int i = 0; i < numQueues; ++i) {
        cout << "Enter quantum for Q" << i << " (-1 for FCFS): ";
        cin >> quantums[i];
    }

    int numProcesses;
    cout << "Enter number of processes: ";
    cin >> numProcesses;

    vector<Process> processes;
    for (int i = 0; i < numProcesses; ++i) {
        string id; int a, b;
        cout << "P" << i+1 << " (ID Arrival Burst): ";
        cin >> id >> a >> b;
        processes.push_back(Process(id, a, b));
    }

    vector<queue<Process*>> mlfq(numQueues);
    string gantt = "";
    int time = 0, completed = 0, timeInQuantum = 0;
    Process* running = nullptr;

    while (completed < numProcesses) {
        // Handle Arrivals
        for (auto& p : processes) {
            if (p.arrivalTime == time) mlfq[0].push(&p);
        }

        // Higher-priority Preemption
        if (running != nullptr) {
            for (int i = 0; i < running->currentQueue; ++i) {
                if (!mlfq[i].empty()) {
                    mlfq[running->currentQueue].push(running);
                    running = nullptr;
                    break;
                }
            }
        }

        // Quantum/Completion Check
        if (running != nullptr) {
            if (quantums[running->currentQueue] != -1 && timeInQuantum == quantums[running->currentQueue]) {
                if (running->currentQueue < numQueues - 1) running->currentQueue++;
                mlfq[running->currentQueue].push(running);
                running = nullptr;
            }
        }

        // Schedule
        if (running == nullptr) {
            for (int i = 0; i < numQueues; ++i) {
                if (!mlfq[i].empty()) {
                    running = mlfq[i].front();
                    mlfq[i].pop();
                    timeInQuantum = 0;
                    break;
                }
            }
        }

        // Execute tick
        if (running != nullptr) {
            if (running->firstRunTime == -1) running->firstRunTime = time;
            gantt += "|" + running->id;
            running->remainingTime--;
            timeInQuantum++;
            if (running->remainingTime == 0) {
                running->completionTime = time + 1;
                completed++;
                running = nullptr;
            }
        } else {
            gantt += "|IDLE";
        }
        time++;
    }

    // Results Display [cite: 52, 60]
    cout << "\nGantt Chart:\n" << gantt << "|\n";
    cout << "\nID\tArr\tBus\tExit\tTAT\tWT\n";
    double sTAT = 0, sWT = 0;
    for (auto& p : processes) {
        int tat = p.completionTime - p.arrivalTime;
        int wt = tat - p.burstTime;
        sTAT += tat; sWT += wt;
        cout << p.id << "\t" << p.arrivalTime << "\t" << p.burstTime << "\t" 
             << p.completionTime << "\t" << tat << "\t" << wt << endl;
    }
    cout << fixed << setprecision(2) << "\nAvg TAT: " << sTAT/numProcesses << "\nAvg WT: " << sWT/numProcesses << endl;

    return 0;
}