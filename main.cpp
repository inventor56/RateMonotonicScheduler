#include <sched.h>
#include <iostream>
#include <chrono>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <signal.h>
#include <atomic>
#include <time.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <mutex>

using namespace std;

// Rate Monotonic Scheduler

//////////////////////////////////////////////////////////////////////////////////////////
// Atomic Flags for protecting shared data (overrun count updating)
//////////////////////////////////////////////////////////////////////////////////////////

atomic<bool> threadT0Finished(false);
atomic<bool> threadT1Finished(false);
atomic<bool> threadT2Finished(false);
atomic<bool> threadT3Finished(false);


bool program_over = false; // Checks if the program should end

///////////////////////////////////
// Variables for program execution
///////////////////////////////////

// Thread attributes
pthread_attr_t attr0; // For main thread
pthread_attr_t attr1; // For T0 thread
pthread_attr_t attr2; // For T1 thread
pthread_attr_t attr3; // For T1 thread
pthread_attr_t attr4; // For T2 thread

// Times to run each doWork (according to the thread it is in)
long runAmntT0 = 1; // Thread 0 runs doWork() 1 time
long runAmntT1 = 2; // Thread 1 runs doWork() 2 times
long runAmntT2 = 4; // Thread 2 runs doWork() 4 times
long runAmntT3 = 16; // Thread 3 runs doWork() 16 times

// Times to run each doWork (according to the thread it is in)
int periodT0 = 1; // Thread 0 runs doWork() 1 time
int periodT1 = 2; // Thread 1 runs doWork() 2 times
int periodT2 = 4; // Thread 2 runs doWork() 4 times
int periodT3 = 16; // Thread 3 runs doWork() 16 times

///////////////
// Timing
///////////////


int framePeriod = 16; // Frame period for scheduler

int programPeriod = 10; // Program period

int nanosecondConversion = 1000000; // This is equal to 1 ms
int periodUnitMS = 10; // 1 unit of time, according to project directions

int runtimeTracker = 0; // Keeps track of how many units of time have gone by for the scheduling program

//////////////////////////////////////////////
// Counter, Deadline trackers, Thread Params
//////////////////////////////////////////////
// Counters for each thread
int counterT0;
int counterT1;
int counterT2;
int counterT3;

// Missed deadlines for each thread
int missedDeadlineT0;
int missedDeadlineT1;
int missedDeadlineT2;
int missedDeadlineT3;

// Scheduling Parameters
sched_param param0;
sched_param param1;
sched_param param2;
sched_param param3;
sched_param param4;


// Pthreads
pthread_t schedulerThread;

// Pthreads for the scheduler. Initially set to 0 (if they exist, you must check to see if they have hit an overrun condition via a flag)
pthread_t T0 = 0;
pthread_t T1 = 0;
pthread_t T2 = 0;
pthread_t T3 = 0;

//CPU
cpu_set_t cpu;

// Semaphore (for scheduler synchronization)
sem_t semScheduler;

// Semaphores (for thread synchronization)
sem_t sem1;
sem_t sem2;
sem_t sem3;
sem_t sem4;

//DoWorkMatrix
int doWorkMatrix[10][10]; // 10x10 matrix for do work

/////////////////////////////////////////////////
// Struct: For passing in specific thread info
/////////////////////////////////////////////////
struct threadValues {
    long* runAmount;
    int* counter;
    sem_t* semaphore;
    atomic<bool>* finished;
};

// Specific struct for this program;
threadValues tValArr[4]; // 4 Thread value structs. One for each of the four threads

/////////////////////////////
// Do Work Function
/////////////////////////////

void doWork() { // Busy work function, multiplies each column of a 10 x 10 matrix (all numbers are 1)
    int total = 1; // total of multiplication
    for(int i = 0; i < 4; i++) { // We go through 5 sets, with each set including 2 columns (the immediate column, then the column five to the right from it)
        for (int j = 0; j < 9; j++) {
            total *= doWorkMatrix[j][i]; // multiply all values in current column
        }
        for (int a = 0; a < 9; a++) {
            total *= doWorkMatrix[a][i + 5]; // now multiply all the values in the column which is five columns over from the current one
        }
    }
}

/////////////////////////////////////////////////////////////
// Timer Handling - Wakes scheduler up periodically
/////////////////////////////////////////////////////////////

// Posts semaphore following time restrictions
void timerHandler(int sig, siginfo_t *si, void *uc )
{
    runtimeTracker++; // Increment tracker
    sem_post(&semScheduler); // Unlock semaphore to allow thread to execute it's doWork() function
}

//////////////////////////////////////
// Threading Function
//////////////////////////////////////

void *run_thread(void * param) {
    //auto time1 = chrono::high_resolution_clock::now();

    while(!program_over) {
        sem_wait(((threadValues*)param)->semaphore);

        *((threadValues*)param)->finished = false; // Thread is not finished
        for (int i = 0; i < *((threadValues*)param)->runAmount; i++) {
            doWork(); // Do busy work
        }
        *((threadValues*)param)->finished = true; // Thread is finished
        *((threadValues*)param)->counter += 1; //Increment respective counter

    }
    pthread_exit(nullptr);
}

/////////////////////////////////////////////////
// Overrun Checking Function
/////////////////////////////////////////////////

bool overrunCheck(bool* firstRunBool) {
    // Check atomic flags and see if there are any overruns
    if (!*firstRunBool && !(*tValArr[0].finished).load())
        missedDeadlineT0++;
    if (!*firstRunBool && !(*tValArr[1].finished).load())
        missedDeadlineT1++;
    if (!*firstRunBool && !(*tValArr[2].finished).load())
        missedDeadlineT2++;
    if (!*firstRunBool && !(*tValArr[3].finished).load())
        missedDeadlineT3++;
}

//////////////////////////////////////
// Rate Monotonic Scheduler
//////////////////////////////////////

void *scheduler(void * param) {

    // Check for if first time through for scheduler (used for overrun checking)
    bool firstRun = true;

    for (int schedulerPeriod = 0; schedulerPeriod < programPeriod; schedulerPeriod++) { // Runs 10 periods
        for (int periodTime = 0; periodTime < framePeriod; periodTime++) { // Runs 16 periods
            //Wait for the timer to signal for the thread to schedule(every 1 unit period)
            sem_wait(&semScheduler);

            //if(periodTime is  at 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 (16 times)

            if (!firstRun && !(*tValArr[0].finished).load())
                missedDeadlineT0++;
            // Post to the respective semaphore, and allow execution of T0
            sem_post(&sem1);
            if (periodTime == 0 || periodTime == 2 || periodTime == 4 || periodTime == 6 || periodTime == 8 ||
                periodTime == 10 || periodTime == 12 || periodTime == 14) { //0,2,4,6,8,10,12,14 (8 times)
                    if (!firstRun && !(*tValArr[1].finished).load())
                        missedDeadlineT1++;
                // Post to the respective semaphore, and allow execution of T1
                sem_post(&sem2);
            }
            if (periodTime == 0 || periodTime == 4 || periodTime == 8 || periodTime == 12) { //0,4,8,12 (4 times)

                if (!firstRun && !(*tValArr[2].finished).load())
                    missedDeadlineT2++;
                // Post to the respective semaphore, and allow execution of T2
                sem_post(&sem3);
            }
            if (periodTime == 0) { //0 (1 time)

                if (!firstRun && !(*tValArr[3].finished).load())
                    missedDeadlineT3++;
                // Post to the respective semaphore, and allow execution of T3
                sem_post(&sem4);
            }

            // Make sure that if the scheduler just started, we don't check for overruns immediately
            if(firstRun)
                firstRun = false;
        }
    }

    // One last overrun check
    overrunCheck(&firstRun);

    pthread_exit(nullptr);
}

int main() {

    /////////////////////////////////////
    // Prompt user for test case || exit
    /////////////////////////////////////

    cout << "Hello and welcome to the Rate Monotonic Scheduler Application." << endl;
    cout << "For Nominal Test Case, enter 1." << endl;
    cout << "For T1 Overrun Test Case, enter 2." << endl;
    cout << "For T2 Overrun Test Case, enter 3." << endl;
    cout << "Enter anything else to exit application.\n" << endl;
    cout << "Please enter your test case number: ";
    int testCase;
    cin >> testCase;

    // Check test case
    if (testCase == 1) {
        // No need to change anything. Start program execution
    }
    else if (testCase == 2) {
        runAmntT1 = 200000000000000000;
    }
    else if (testCase == 3)
        runAmntT2 = 4000000000000000000;
    else {
        exit(1);
    }


    /////////////////////////////////////////////////
    // Program setup and initialization
    /////////////////////////////////////////////////

    // Initialize do work matrix
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            doWorkMatrix[i][j] = 1; // Set all matrix values to one
        }
    }

    //Initialize scheduler semaphore (shared and locked to start)
    sem_init(&semScheduler, 1, 0);

    //Initialize thread semaphores (all shared and all locked to start)
    sem_init(&sem1, 1, 0);
    sem_init(&sem2, 1, 0);
    sem_init(&sem3, 1, 0);
    sem_init(&sem4, 1, 0);

    // Initialize counters
    counterT0 = 0;
    counterT1 = 0;
    counterT2 = 0;
    counterT3 = 0;

    // Initialize deadlines
    missedDeadlineT0 = 0;
    missedDeadlineT1 = 0;
    missedDeadlineT2 = 0;
    missedDeadlineT3 = 0;


    // Set CPU priority to be the same for all threads;
    CPU_ZERO(&cpu);
    CPU_SET(0, &cpu); //  All threads should run on CPU 1


    // Initialize thread attributes
    pthread_attr_init(&attr0);
    pthread_attr_init(&attr1);
    pthread_attr_init(&attr2);
    pthread_attr_init(&attr3);
    pthread_attr_init(&attr4);

    // Set processor affinity to be the same for all threads
    pthread_attr_setaffinity_np(&attr0, sizeof(cpu_set_t), &cpu); // Set processor affinity;
    pthread_attr_setaffinity_np(&attr1, sizeof(cpu_set_t), &cpu); // Set processor affinity;
    pthread_attr_setaffinity_np(&attr2, sizeof(cpu_set_t), &cpu); // Set processor affinity;
    pthread_attr_setaffinity_np(&attr3, sizeof(cpu_set_t), &cpu); // Set processor affinity;
    pthread_attr_setaffinity_np(&attr4, sizeof(cpu_set_t), &cpu); // Set processor affinity;

    ///////////////////////////////////////
    // Setting priorities for each thread!
    ///////////////////////////////////////

    param0.__sched_priority = sched_get_priority_max(SCHED_FIFO); // Max Priority for the RMS scheduler (99)
    param1.__sched_priority = 90; // 2nd highest priority
    param2.__sched_priority = 80; // 3rd highest priority
    param3.__sched_priority = 70; // 4th highest priority
    param4.__sched_priority = 60; // Lowest priority (it's period is the shortest)

    // Set Policy
    pthread_attr_setschedpolicy(&attr0, SCHED_FIFO);
    pthread_attr_setschedpolicy(&attr1, SCHED_FIFO);
    pthread_attr_setschedpolicy(&attr2, SCHED_FIFO);
    pthread_attr_setschedpolicy(&attr3, SCHED_FIFO);
    pthread_attr_setschedpolicy(&attr4, SCHED_FIFO);

    // Set thread priority
    pthread_attr_setschedparam(&attr0, &param0);
    pthread_attr_setschedparam(&attr1, &param1);
    pthread_attr_setschedparam(&attr2, &param2);
    pthread_attr_setschedparam(&attr3, &param3);
    pthread_attr_setschedparam(&attr4, &param4);

    // Oh! You need to make sure you use the attributes object!
    pthread_attr_setinheritsched(&attr0, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setinheritsched(&attr1, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setinheritsched(&attr2, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setinheritsched(&attr3, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setinheritsched(&attr4, PTHREAD_EXPLICIT_SCHED);

    //Put in counters
    tValArr[0].counter = &counterT0;
    tValArr[1].counter = &counterT1;
    tValArr[2].counter = &counterT2;
    tValArr[3].counter = &counterT3;

    // Put in run time amount
    tValArr[0].runAmount = &runAmntT0;
    tValArr[1].runAmount = &runAmntT1;
    tValArr[2].runAmount = &runAmntT2;
    tValArr[3].runAmount = &runAmntT3;

    // Put in semaphores
    tValArr[0].semaphore = &sem1;
    tValArr[1].semaphore = &sem2;
    tValArr[2].semaphore = &sem3;
    tValArr[3].semaphore = &sem4;

    // Atmoic bool
    tValArr[0].finished = &threadT0Finished;
    tValArr[1].finished = &threadT1Finished;
    tValArr[2].finished = &threadT2Finished;
    tValArr[3].finished = &threadT3Finished;

    // Create pthreads here in main thread - Semaphores will syncronize them, as the scheduler will dispatch (unlock) each one accordingly
    pthread_create(&T0, &attr1, run_thread, (void *) &tValArr[0]);
    pthread_create(&T1, &attr2, run_thread, (void *) &tValArr[1]);
    pthread_create(&T2, &attr3, run_thread, (void *) &tValArr[2]);
    pthread_create(&T3, &attr4, run_thread, (void *) &tValArr[3]);

    // CREATE SCHEDULER
    int tidSchThr = pthread_create(&schedulerThread, &attr0, scheduler, nullptr);


    ////////////////////////
    // Set up and run timer
    ///////////////////////

    long oneUnitOfTime = nanosecondConversion*periodUnitMS;
    long totalRuntime = oneUnitOfTime*programPeriod*nanosecondConversion; // 1 unit(10 ms) x 16 unit frame period  x 10 unit program period


    struct sigevent sig;
    struct sigaction sa;
    int sigNo = SIGRTMIN;

    timer_t unitTimer;
    itimerspec its;

    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = timerHandler;
    sigemptyset(&sa.sa_mask);

    sigaction(sigNo, &sa, nullptr);

    sig.sigev_notify = SIGEV_SIGNAL;
    sig.sigev_signo = sigNo;
    sig.sigev_value.sival_ptr = nullptr;

    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = oneUnitOfTime; // Expiration time  oneUnitOfTime
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = its.it_value.tv_nsec; // Same as above and it repeats  its.it_value.tv_nsec

    //Create Timer
    timer_create(CLOCK_REALTIME, &sig, &unitTimer);

    if (timer_settime(unitTimer, 0, &its, nullptr) == -1) // Start Timer
        cout << "FAILURE" << endl;


    ///////////////////////////////////////////////////////
    // Join scheduler thread at end of program execution
    ///////////////////////////////////////////////////////

    pthread_join(schedulerThread, nullptr);


    ///////////////////////////////////////////////////////
    // Program is terminated
    ///////////////////////////////////////////////////////

    program_over = true; // program is done

    // Print out results
    cout << "T0 Ran " << counterT0 << " Times" << endl;
    cout << "T1 Ran " << counterT1 << " Times" << endl;
    cout << "T2 Ran " << counterT2 << " Times" << endl;
    cout << "T3 Ran " << counterT3 << " Times\n" << endl;

    // Print out results
    cout << "T0 had " << missedDeadlineT0 << " missed deadlines" << endl;
    cout << "T1 had " << missedDeadlineT1 << " missed deadlines" << endl;
    cout << "T2 had " << missedDeadlineT2 << " missed deadlines" << endl;
    cout << "T3 had " << missedDeadlineT3 << " missed deadlines" << endl;

    return 0;
}
