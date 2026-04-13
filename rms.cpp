#include <iostream>
#include <vector>
#include <string>
#include <numeric>
#include <algorithm>
#include <iomanip>
#include <cmath>

using namespace std;

// Structure for a real-time periodic process
struct Process {
    string id;
    int arrivalTime;
    int burstTime;
    int period; // In RMS, Deadline is assumed to be equal to Period
    int remainingTime;
    int nextDeadline;
    int missedDeadlines;

    Process(string id, int arrival, int burst, int p) : 
        id(id), arrivalTime(arrival), burstTime(burst), period(p), 
        remainingTime(0), nextDeadline(arrival + p), missedDeadlines(0) {}
};

int gcd(int a, int b) {
    return b == 0 ? a : gcd(b, a % b);
}

// Calculate LCM for the Hyperperiod
int getLcm(int a, int b) {
    return (a * b) / gcd(a, b);
}

int main() {
    int n;
    cout << "Enter number of processes: ";
    cin >> n;

    vector<Process> processes;
    int hyperperiod = 1;

    cout << "Enter Process_ID Arrival_Time Burst_Time Period/Deadline:\n";
    for (int i = 0; i < n; i++) {
        string id;
        int a, b, p;
        cin >> id >> a >> b >> p;
        processes.push_back(Process(id, a, b, p));
        
        // Calculate the Hyperperiod (LCM of all periods)
        if (i == 0) {
            hyperperiod = p;
        } else {
            hyperperiod = getLcm(hyperperiod, p);
        }
    }

    // Stores execution history: <Time_Second, Process_ID>
    vector<pair<int, string>> execution_log;

    cout << "\n--- Starting RMS Simulation for Hyperperiod: " << hyperperiod << " ---\n";

    // Simulation Loop (second by second)
    for (int t = 0; t < hyperperiod; t++) {
        
        // Check for job releases and deadline misses
        for (auto& p : processes) {
            if (t >= p.arrivalTime && (t - p.arrivalTime) % p.period == 0) {
                if (p.remainingTime > 0) {
                    p.missedDeadlines++; // Previous job missed deadline
                }
                p.remainingTime = p.burstTime;
                p.nextDeadline = t + p.period;
            }
        }

        // Find ready process with highest priority (RMS = Shortest Period)
        int highestPriorityIdx = -1;
        int minPeriod = 1e9;

        for (int i = 0; i < n; i++) {
            if (processes[i].remainingTime > 0 && processes[i].arrivalTime <= t) {
                if (processes[i].period < minPeriod) {
                    minPeriod = processes[i].period;
                    highestPriorityIdx = i;
                }
            }
        }

        // Execute chosen process or go IDLE
        if (highestPriorityIdx != -1) {
            processes[highestPriorityIdx].remainingTime--;
            execution_log.push_back({t, processes[highestPriorityIdx].id});
        } else {
            execution_log.push_back({t, "IDLE"});
        }
    }

    // Output Execution Timeline (Gantt Chart equivalent)
    cout << "\nExecution Log (CPU Timeline):\n";
    cout << "Time\t\tProcess in CPU\n";
    cout << "------------------------------\n";
    for (auto const& log : execution_log) {
        cout << "[" << log.first << " -> " << log.first + 1 << "]\t" << log.second << "\n";
    }

    // Performance & Schedulability Report
    cout << "\n--- Performance & Schedulability Report ---\n";
    double utilization = 0;
    
    for (const auto& p : processes) {
        utilization += (double)p.burstTime / p.period;
        cout << "Process " << p.id << " Missed Deadlines: " << p.missedDeadlines << "\n";
    }
    
    // Theoretical Bound Check: U <= n(2^(1/n) - 1)
    double theoreticalBound = n * (pow(2.0, 1.0 / n) - 1);
    
    cout << fixed << setprecision(4);
    cout << "Total CPU Utilization: " << utilization * 100 << "%\n";
    cout << "RMS Schedulability Bound: " << theoreticalBound * 100 << "%\n";
    
    if (utilization <= theoreticalBound) {
        cout << "Status: Schedulable (Utilization <= Bound)\n";
    } else if (utilization <= 1.0) {
        cout << "Status: Further Analysis Required (Bound < Utilization <= 100%)\n";
    } else {
        cout << "Status: Not Schedulable (Utilization > 100%)\n";
    }

    return 0;
}
