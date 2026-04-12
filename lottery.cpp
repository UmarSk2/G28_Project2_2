#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <iomanip>

using namespace std;

struct Process {
    string id;
    int arrivalTime;
    int burstTime;
    int remainingTime;
    int tickets;
    int completionTime;
    int waitingTime;
    int turnaroundTime;
};

int main() {
    srand(time(0)); // Seed for randomness
    int numProcesses, timeSlice;
    
    cout << "Enter Number of Processes: ";
    cin >> numProcesses;
    cout << "Enter Time Quantum: ";
    cin >> timeSlice;

    vector<Process> proc(numProcesses);
    int totalBurst = 0;

    for (int i = 0; i < numProcesses; i++) {
        cout << "Enter ID, Arrival, Burst, and Tickets for P" << i + 1 << ": ";
        cin >> proc[i].id >> proc[i].arrivalTime >> proc[i].burstTime >> proc[i].tickets;
        proc[i].remainingTime = proc[i].burstTime;
        totalBurst += proc[i].burstTime;
    }

    int currentTime = 0;
    int completed = 0;
    string ganttChart = "";

    while (completed < numProcesses) {
        int totalTicketsAvailable = 0;
        vector<int> readyIndices;

        // Check which processes have arrived and are not finished
        for (int i = 0; i < numProcesses; i++) {
            if (proc[i].arrivalTime <= currentTime && proc[i].remainingTime > 0) {
                totalTicketsAvailable += proc[i].tickets;
                readyIndices.push_back(i);
            }
        }

        if (totalTicketsAvailable == 0) {
            ganttChart += "[Idle]";
            currentTime++;
            continue;
        }

        // Draw the winning ticket
        int winningTicket = rand() % totalTicketsAvailable;
        int ticketSum = 0;
        int winnerIdx = -1;

        for (int idx : readyIndices) {
            ticketSum += proc[idx].tickets;
            if (winningTicket < ticketSum) {
                winnerIdx = idx;
                break;
            }
        }

        // Execute the winner for a time slice
        int runTime = min(proc[winnerIdx].remainingTime, timeSlice);
        for(int t=0; t<runTime; t++) ganttChart += "|" + proc[winnerIdx].id;
        
        proc[winnerIdx].remainingTime -= runTime;
        currentTime += runTime;

        if (proc[winnerIdx].remainingTime == 0) {
            proc[winnerIdx].completionTime = currentTime;
            proc[winnerIdx].turnaroundTime = proc[winnerIdx].completionTime - proc[winnerIdx].arrivalTime;
            proc[winnerIdx].waitingTime = proc[winnerIdx].turnaroundTime - proc[winnerIdx].burstTime;
            completed++;
        }
    }

    // Output Visual Schedule and Metrics
    cout << "\nVisual Gantt Chart:\n" << ganttChart << "|\n";
    cout << "\nID\tArrival\tBurst\tTickets\tExit\tTurnaround\tWaiting\n";
    float totalWT = 0, totalTAT = 0;
    for (auto p : proc) {
        cout << p.id << "\t" << p.arrivalTime << "\t" << p.burstTime << "\t" 
             << p.tickets << "\t" << p.completionTime << "\t" 
             << p.turnaroundTime << "\t\t" << p.waitingTime << endl;
        totalWT += p.waitingTime;
        totalTAT += p.turnaroundTime;
    }

    cout << fixed << setprecision(2);
    cout << "\nAverage Waiting Time: " << totalWT / numProcesses;
    cout << "\nAverage Turnaround Time: " << totalTAT / numProcesses << endl;

    return 0;
}