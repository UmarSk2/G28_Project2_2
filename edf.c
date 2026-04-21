#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdbool.h>

// Structure to represent a process
typedef struct {
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
} Process;

// Global simulation variables
Process *processes;
int num_processes;
pthread_mutex_t lock_mutex = PTHREAD_MUTEX_INITIALIZER;
int current_time = 0;
bool simulation_active = true;
int num_processors = 2;

// Log to store execution history: cpu_log[cpu_index][time_tick]
int **cpu_log; 
int log_capacity = 1000; // Initial capacity for the timeline log
int current_log_size = 0;

void handle_signal(int sig) {
    if (sig == SIGUSR1 || sig == SIGINT) {
        simulation_active = false;
    }
}

// Thread function for the scheduler
void* scheduler_tick(void* arg) {
    while (simulation_active) {
        pthread_mutex_lock(&lock_mutex);
        
        bool all_done = true;
        for (int i = 0; i < num_processes; i++) {
            if (!processes[i].is_completed) {
                all_done = false;
                break;
            }
        }
        
        if (all_done) {
            simulation_active = false;
            pthread_mutex_unlock(&lock_mutex);
            break;
        }

        // Handle I/O decrement
        for (int i = 0; i < num_processes; i++) {
            if (processes[i].in_io) {
                processes[i].remaining_io--;
                if (processes[i].remaining_io <= 0) {
                    processes[i].in_io = false;
                }
            }
        }

        // Find available processes
        int *available = malloc(num_processes * sizeof(int));
        int avail_count = 0;
        for (int i = 0; i < num_processes; i++) {
            if (!processes[i].is_completed && processes[i].arrival_time <= current_time && !processes[i].in_io) {
                available[avail_count++] = i;
            }
        }

        // Simple Selection Sort for EDF (Earliest Deadline First)
        for (int i = 0; i < avail_count - 1; i++) {
            for (int j = i + 1; j < avail_count; j++) {
                if (processes[available[i]].deadline > processes[available[j]].deadline) {
                    int temp = available[i];
                    available[i] = available[j];
                    available[j] = temp;
                }
            }
        }

        // Assign to processors
        for (int cpu = 0; cpu < num_processors; cpu++) {
            if (cpu < avail_count) {
                int p_idx = available[cpu];
                
                if (processes[p_idx].start_time == -1) 
                    processes[p_idx].start_time = current_time;
                
                processes[p_idx].remaining_time--;
                cpu_log[cpu][current_time] = processes[p_idx].id;
                
                // Simulate I/O burst halfway through
                if (processes[p_idx].remaining_time > 0 && 
                    processes[p_idx].remaining_time == processes[p_idx].execution_time / 2 && 
                    processes[p_idx].io_time > 0) {
                    processes[p_idx].in_io = true;
                } else if (processes[p_idx].remaining_time == 0) {
                    processes[p_idx].completion_time = current_time + 1;
                    processes[p_idx].is_completed = true;
                }
            } else {
                cpu_log[cpu][current_time] = 0; // IDLE
            }
        }

        current_time++;
        current_log_size = current_time;
        
        // Dynamic realloc if log exceeds capacity
        if (current_time >= log_capacity - 1) {
            log_capacity *= 2;
            for(int i = 0; i < num_processors; i++) {
                cpu_log[i] = realloc(cpu_log[i], log_capacity * sizeof(int));
            }
        }

        free(available);
        pthread_mutex_unlock(&lock_mutex);
        usleep(100);
    }
    return NULL;
}

int main() {
    signal(SIGUSR1, handle_signal);
    signal(SIGINT, handle_signal);
    
    printf("Enter number of processors: ");
    if (scanf("%d", &num_processors) != 1) return 1;

    printf("Enter number of processes: ");
    if (scanf("%d", &num_processes) != 1) return 1;

    processes = malloc(num_processes * sizeof(Process));
    cpu_log = malloc(num_processors * sizeof(int*));
    for(int i = 0; i < num_processors; i++) {
        cpu_log[i] = calloc(log_capacity, sizeof(int));
    }

    printf("Enter details (Process_ID Arrival_Time Burst_Time Deadline):\n");
    for (int i = 0; i < num_processes; i++) {
        char p_name[10];
        int at, bt, dl;
        scanf("%s %d %d %d", p_name, &at, &bt, &dl);
        
        int id = (p_name[0] == 'P' || p_name[0] == 'p') ? atoi(&p_name[1]) : atoi(p_name);

        processes[i].id = id;
        processes[i].execution_time = bt;
        processes[i].io_time = 0;
        processes[i].deadline = dl;
        processes[i].arrival_time = at;
        processes[i].remaining_time = bt;
        processes[i].remaining_io = 0;
        processes[i].start_time = -1;
        processes[i].completion_time = 0;
        processes[i].is_completed = false;
        processes[i].in_io = false;
    }

    pthread_t sched_thread;
    pthread_create(&sched_thread, NULL, scheduler_tick, NULL);
    pthread_join(sched_thread, NULL);

    printf("\nP\tAT\tBT\tDL\tCT\tTAT\tWT\tMiss\n");
    float total_tat = 0, total_wt = 0;
    int misses = 0;

    for (int i = 0; i < num_processes; i++) {
        int tat = processes[i].completion_time - processes[i].arrival_time;
        int wt = tat - processes[i].execution_time;
        if (wt < 0) wt = 0;
        bool miss = processes[i].completion_time > processes[i].deadline;
        
        total_tat += tat;
        total_wt += wt;
        if (miss) misses++;

        printf("P%d\t%d\t%d\t%d\t%d\t%d\t%d\t%s\n", 
               processes[i].id, processes[i].arrival_time, processes[i].execution_time,
               processes[i].deadline, processes[i].completion_time, tat, wt, miss ? "Yes" : "No");
    }

    printf("\nMetrics:\nAvg TAT: %.2f\nAvg WT: %.2f\nDeadline Misses: %d\n\n", 
           total_tat / num_processes, total_wt / num_processes, misses);

    for (int cpu = 0; cpu < num_processors; cpu++) {
        printf("CPU %d Exec Graph:\n|", cpu);
        for (int i = 0; i < current_log_size; i++) {
            if (cpu_log[cpu][i] == 0) printf(" IDLE |");
            else printf("  P%d  |", cpu_log[cpu][i]);
        }
        printf("\n0");
        for (int i = 1; i <= current_log_size; i++) {
            printf("%7d", i);
        }
        printf("\n\n");
    }

    // Cleanup
    for(int i = 0; i < num_processors; i++) free(cpu_log[i]);
    free(cpu_log);
    free(processes);

    return 0;
}
