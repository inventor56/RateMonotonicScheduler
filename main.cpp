#include <sched.h>
#include <iostream>
#include <chrono>
#include <cstdio>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <csignal>
#include <ctime>
#include <sys/sysinfo.h>

using namespace std;
// Rate Monotonic Scheduler


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

// Frame period for scheduler
int framePeriod = 16;

// Program period
int programPeriod = 10;

int nanosecondConversion = 1000000; // This is equal to 1 ms
int periodUnitMS = 10; // 1 unit of time, according to project directions

////////////////////////////////////////////

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

//////////////////////////////////////
// Threading Function
//////////////////////////////////////

void *run_thread(void * param) {

    /*struct threadValues *passedInValues;
    passedInValues = (threadValues*) param; */
    bool test = true;
    //auto time1 = chrono::high_resolution_clock::now();

    while(true) {
        sem_wait(((threadValues*)param)->semaphore);
        for (int i = 0; i < *((threadValues*)param)->runAmount; i++) {
            doWork(); // Do busy work
        }
        *((threadValues*)param)->counter += 1; //Increment respective counter

        // We don't want to have the sleep here, this is just for testing purposes
        //sleep(1);

        if (*((threadValues*)param)->runAmount == runAmntT0)
            cout << "This thread is T0. It is running on CPU: " << sched_getcpu() << endl;
        if (*((threadValues*)param)->runAmount == runAmntT1)
            cout << "This thread is T1. It is running on CPU: "  << sched_getcpu() << endl;
        if (*((threadValues*)param)->runAmount == runAmntT2)
            cout << "This thread is T2. It is running on CPU: "  << sched_getcpu() << endl;
        if (*((threadValues*)param)->runAmount == runAmntT3)
            cout << "This thread is T3. It is running on CPU: " << sched_getcpu() << endl;

        /*
        auto time2 = chrono::high_resolution_clock::now();
        auto wms_conversion = chrono::duration_cast<chrono::milliseconds>(time2 - time1);
        chrono::duration<double, milli> fms_conversion = (time2 - time1);
        cout << wms_conversion.count() << " whole seconds" << endl;
        cout << fms_conversion.count() << " milliseconds" << endl; */
    }



    pthread_exit(nullptr);
}

//////////////////////////////////////
// Rate Monotonic Scheduler
//////////////////////////////////////

void *scheduler(void * param) {

    sem_wait(&semScheduler); //
    for (int periodTime = 0; periodTime < framePeriod; periodTime++) {
        sleep(1);
        //sem_wait(&semScheduler); // Wait until timer kicks in

        // Check flags and see if there are any overruns
        //if(T0 != 0 && )
            //cout << "eurakeua first time" << endl;

        //if(periodTime % ) 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 (16 times)
        // Post to the respective semaphore, and allow execution of T0
        sem_post(&sem1);
        if(periodTime == 0 || periodTime == 2 || periodTime == 4 || periodTime == 6 || periodTime == 8 || periodTime == 10 || periodTime == 12 || periodTime == 14) { //0,2,4,6,8,10,12,14 (8 times)
            // Post to the respective semaphore, and allow execution of T1
            sem_post(&sem2);
        }
        if(periodTime == 0 || periodTime == 4 || periodTime == 8 || periodTime == 12) { //0,4,8,12 (4 times)
            // Post to the respective semaphore, and allow execution of T2
            sem_post(&sem3);
        }
        if(periodTime == 0) { //0 (1 time)
            // Post to the respective semaphore, and allow execution of T3
            sem_post(&sem4);
        }

    }

    /* Kick off all four threads
    int tid0 = pthread_create(&T0, &attr1, run_thread, (void *) &tValArr[0]);


    int tid1 = pthread_create(&T1, &attr2, run_thread, (void *) &tValArr[1]);

    int tid2 = pthread_create(&T2, &attr3, run_thread, (void *) &tValArr[2]);

    int tid3 = pthread_create(&T3, &attr4, run_thread, (void *) &tValArr[3]); */

    pthread_exit(nullptr);
}

int main() {

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

    // May need to swap these and put them below setschedparam
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

    // Oh! You need to make sure you use the attributes object! This is where the problem lies
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


    sigevent sig;

    timer_t intervalTimer;
    itimerspec its;

    sig.sigev_notify = SIGEV_NONE;
    its.it_value.tv_nsec = periodUnitMS*programPeriod*framePeriod*nanosecondConversion; // Expiration time
    its.it_interval.tv_nsec = its.it_value.tv_nsec; // Same as above and it repeats

    //timer_create(CLOCK_REALTIME, &sig, &intervalTimer);

    // Create pthreads here in main thread - Semaphores will syncronize them, as the scheduler will dispatch (unlock) each one accordingly
    pthread_create(&T0, &attr1, run_thread, (void *) &tValArr[0]);
    pthread_create(&T1, &attr2, run_thread, (void *) &tValArr[1]);
    pthread_create(&T2, &attr3, run_thread, (void *) &tValArr[2]);
    pthread_create(&T3, &attr4, run_thread, (void *) &tValArr[3]);



    // CREATE SCHEDULER
    int tidSchThr = pthread_create(&schedulerThread, &attr0, scheduler, nullptr);

    sem_post(&semScheduler);

    // Start timer
    // while (timer < 160*100 ms)
    //
    //      sem_post(&semScheduler)

    //timer_settime(intervalTimer, 0, &its, nullptr);

    // Join scheduler thread at end of program execution
    pthread_join(schedulerThread, nullptr);


    // Print out results
    cout << "T0 Ran " << counterT0 << " Times" << endl;
    cout << "T1 Ran " << counterT1 << " Times" << endl;
    cout << "T2 Ran " << counterT2 << " Times" << endl;
    cout << "T3 Ran " << counterT3 << " Times" << endl;


    return 0;
}
