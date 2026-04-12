#include <iostream>
#include <vector>
#include <pthread.h>
#include <unistd.h>
#include <csignal>
#include <iomanip>
#include <algorithm>
#include <string>

using namespace std;

// Structure to represent a process and its parameters/state
struct Process {
    int id;
    int execution_time;
    int io_time;
    int deadline;
    int arrival_time;
    int remaining_time;
    int remaining_io;
    int start_time;
    int completion_time;
    bool is_completed;
    bool in_io;
};

vector<Process> processes;
pthread_mutex_t lock_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex for thread synchronization
int current_time = 0;
bool simulation_active = true;
int num_processors = 2;
vector<vector<int>> cpu_log; // Stores execution history for each CPU to generate the graph

// Signal handler to gracefully stop the simulation on interrupts
void handle_signal(int sig) {
    if (sig == SIGUSR1 || sig == SIGINT) {
        simulation_active = false;
    }
}

// Thread function to simulate the scheduler's clock ticks
void* scheduler_tick(void* arg) {
    while (simulation_active) {
        pthread_mutex_lock(&lock_mutex);
        
        // Check if all processes have finished execution
        bool all_done = true;
        for (auto& p : processes) {
            if (!p.is_completed) {
                all_done = false; 
                break;
            }
        }
        
        // Exit loop if simulation is complete
        if (all_done) {
            simulation_active = false;
            pthread_mutex_unlock(&lock_mutex);
            break;
        }

        // Decrement I/O time for processes currently in their I/O burst
        for (auto& p : processes) {
            if (p.in_io) {
                p.remaining_io--;
                if (p.remaining_io <= 0) {
                    p.in_io = false; // Process finishes I/O and returns to ready state
                }
            }
        }

        // Identify processes that have arrived, are not completed, and are not in I/O
        vector<int> available_processes;
        for (size_t i = 0; i < processes.size(); ++i) {
            if (!processes[i].is_completed && processes[i].arrival_time <= current_time && !processes[i].in_io) {
                available_processes.push_back(i);
            }
        }

        // Earliest Deadline First (EDF) Logic: Sort available processes by their absolute deadline
        sort(available_processes.begin(), available_processes.end(), [](int a, int b) {
            return processes[a].deadline < processes[b].deadline;
        });

        // Assign top priority processes to available processors
        int p_idx = 0;
        for (int cpu = 0; cpu < num_processors; ++cpu) {
            if (p_idx < available_processes.size()) {
                int p = available_processes[p_idx];
                
                // Record start time on first dispatch
                if (processes[p].start_time == -1) processes[p].start_time = current_time;
                
                processes[p].remaining_time--;
                cpu_log[cpu].push_back(processes[p].id); // Log execution for the timeline graph
                
                // Simulate I/O burst occurring halfway through execution (if applicable)
                if (processes[p].remaining_time > 0 && processes[p].remaining_time == processes[p].execution_time / 2 && processes[p].io_time > 0 && processes[p].remaining_io > 0) {
                    processes[p].in_io = true;
                } else if (processes[p].remaining_time == 0) {
                    // Mark process as completed and record completion time
                    processes[p].completion_time = current_time + 1;
                    processes[p].is_completed = true;
                }
                p_idx++;
            } else {
                // Processor remains idle if no processes are available
                cpu_log[cpu].push_back(0);
            }
        }

        current_time++; // Advance the simulation clock by 1 unit
        pthread_mutex_unlock(&lock_mutex);
        usleep(100); // Sleep to simulate processing delay
    }
    return NULL;
}

int main() {
    // Register signal handlers for manual termination
    signal(SIGUSR1, handle_signal);
    signal(SIGINT, handle_signal);
    
    // Accept multiprocessor configuration
    cout << "Enter number of processors: ";
    cin >> num_processors;
    cpu_log.resize(num_processors);

    int n;
    cout << "Enter number of processes: ";
    cin >> n;

    // Accept process scheduling parameters from the user
    cout << "Enter details (Process_ID Arrival_Time Burst_Time Deadline):\n";
    for (int i = 0; i < n; ++i) {
        string p_name;
        int at, bt, dl;
        cin >> p_name >> at >> bt >> dl;
        
        // Handle both 'P1' and '1' formats for Process ID
        int id = 0;
        if(p_name[0] == 'P' || p_name[0] == 'p') {
            id = stoi(p_name.substr(1));
        } else {
            id = stoi(p_name);
        }

        // Initialize process struct and add to the queue (I/O defaults to 0 for standard input)
        processes.push_back({id, bt, 0, dl, at, bt, 0, -1, 0, false, false});
    }

    // Initialize and start the EDF scheduler thread
    pthread_t sched_thread;
    pthread_create(&sched_thread, NULL, scheduler_tick, NULL);
    
    // Wait for the scheduling simulation to complete
    pthread_join(sched_thread, NULL);

    // Calculate and display performance metrics in a tabular format
    cout << "\nP\tAT\tBT\tIO\tDL\tCT\tTAT\tWT\tMiss\n";
    float total_tat = 0, total_wt = 0;
    int misses = 0;

    for (auto& p : processes) {
        int tat = p.completion_time - p.arrival_time; // Turnaround Time
        int wt = tat - p.execution_time - p.io_time;  // Waiting Time
        if (wt < 0) wt = 0;
        bool miss = p.completion_time > p.deadline;   // Determine if the real-time deadline was missed
        
        total_tat += tat;
        total_wt += wt;
        if (miss) misses++;

        cout << "P" << p.id << "\t" << p.arrival_time << "\t" << p.execution_time << "\t"
             << p.io_time << "\t" << p.deadline << "\t" << p.completion_time << "\t"
             << tat << "\t" << wt << "\t" << (miss ? "Yes" : "No") << "\n";
    }

    // Print summary averages
    cout << "\nMetrics:\n";
    if (!processes.empty()) {
        cout << "Avg TAT: " << fixed << setprecision(2) << total_tat / processes.size() << "\n";
        cout << "Avg WT: " << total_wt / processes.size() << "\n";
    }
    cout << "Deadline Misses: " << misses << "\n\n";

    // Render the execution timeline graph for each CPU
    for (int cpu = 0; cpu < num_processors; ++cpu) {
        cout << "CPU " << cpu << " Exec Graph:\n|";
        for (int id : cpu_log[cpu]) {
            if (id == 0) cout << " IDLE |";
            else cout << "  P" << id << "  |";
        }
        cout << "\n0"; // Timeline base
        for (size_t i = 1; i <= cpu_log[cpu].size(); ++i) {
            cout << setw(7) << i; // Align timeline numbers with graph boxes
        }
        cout << "\n\n";
    }

    return 0;
}