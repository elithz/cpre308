/**
*		    Filename:  scheduling.c
*    Description:  four different process scheduling algorithms implement
*        Version:  1.0
*        Created:  10.26.2017 22h05min23s
*         Author:  Ningyuan Zhang （狮子劫博丽）(elithz), elithz@iastate.edu 
*        Company:  NERVE Software
*/

#include <stdio.h>
#include <string.h>

#define NUM_PROCESSES 20

struct process
{
  /* Values initialized for each process */
  int arrivaltime; /* Time process arrives and wishes to start */
  int runtime;     /* Time process requires to complete job */
  int priority;    /* Priority of the process */

  /* Values algorithm may use to track processes */
  int starttime;
  int endtime;
  int flag;
  int remainingtime;
};

/* Forward declarations of Scheduling algorithms */
void first_come_first_served(struct process *proc);
void shortest_remaining_time(struct process *proc);
void round_robin(struct process *proc);
void round_robin_priority(struct process *proc);

int main()
{
  int i;
  struct process proc[NUM_PROCESSES], /* List of processes */
      proc_copy[NUM_PROCESSES];       /* Backup copy of processes */

  /* Seed random number generator */
  /*srand(time(0));*/ /* Use this seed to test different scenarios */
  srand(0xC0FFEE);    /* Used for test to be printed out */

  /* Initialize process structures */
  for (i = 0; i < NUM_PROCESSES; i++)
  {
    proc[i].arrivaltime = rand() % 100;
    proc[i].runtime = (rand() % 30) + 10;
    proc[i].priority = rand() % 3;
    proc[i].starttime = 0;
    proc[i].endtime = 0;
    proc[i].flag = 0;
    proc[i].remainingtime = 0;
  }

  /* Show process values */
  printf("Process\tarrival\truntime\tpriority\n");
  for (i = 0; i < NUM_PROCESSES; i++)
    printf("%d\t%d\t%d\t%d\n", i, proc[i].arrivaltime, proc[i].runtime,
           proc[i].priority);

  /* Run scheduling algorithms */
  printf("\n\nFirst come first served\n");
  memcpy(proc_copy, proc, NUM_PROCESSES * sizeof(struct process));
  first_come_first_served(proc_copy);

  printf("\n\nShortest remaining time\n");
  memcpy(proc_copy, proc, NUM_PROCESSES * sizeof(struct process));
  shortest_remaining_time(proc_copy);

  printf("\n\nRound Robin\n");
  memcpy(proc_copy, proc, NUM_PROCESSES * sizeof(struct process));
  round_robin(proc_copy);

  printf("\n\nRound Robin with priority\n");
  memcpy(proc_copy, proc, NUM_PROCESSES * sizeof(struct process));
  round_robin_priority(proc_copy);

  return 0;
}

void first_come_first_served(struct process *proc)
{
  /* Implement scheduling algorithm here */
  //counters
  int i, j;
  //running total of completion time
  int totalComRunTime = 0;
  //average completion time
  int avgComRunTime;
  //system time
  int sysTime = 0;
  //track of first come
  int firstCome;
  //through num processes to execute them
  for (i = 0; i < NUM_PROCESSES; i++)
  {
    //initialize firstCome
    firstCome = i;
    //find first come
    for (j = 0; j < NUM_PROCESSES; j++)
    {
      //if firstCome is already complete then overwrite it
      if (proc[firstCome].flag)
        firstCome = j;

      //if firstCome arrival time is after current process
      //and current process hasn't been executed
      //then overwrite it
      else if (proc[firstCome].arrivaltime > proc[j].arrivaltime && !proc[j].flag)
        firstCome = j;
    }

    //advance system time if it hasn't caught up to arrival time
    if (sysTime < proc[firstCome].arrivaltime)
      sysTime = proc[firstCome].arrivaltime;

    //execute process
    //set firstCome start time
    proc[firstCome].starttime = sysTime;
    //advance system time
    sysTime += proc[firstCome].runtime;
    //set firstCome end time
    proc[firstCome].endtime = sysTime;
    //track of completion time running total
    totalComRunTime += (proc[firstCome].endtime - proc[firstCome].arrivaltime);
    //mark firstCome as completed
    proc[firstCome].flag = 1;

    printf("Process %d started at time %d\n", firstCome, proc[firstCome].starttime);
    printf("Process %d finished at time %d\n", firstCome, proc[firstCome].endtime);
  }

  //calculate average completion time
  avgComRunTime = totalComRunTime / NUM_PROCESSES;

  printf("Average time from arrival to completion is %d seconds\n", avgComRunTime);
}

void shortest_remaining_time(struct process *proc)
{
  /* Implement scheduling algorithm here */
  //counters
  int i, j;
  //running total of completion time
  int totalComRunTime = 0;
  //average completion time
  int avgComRunTime;
  //system time
  int sysTime = 0;
  //track of first come
  int shortestRemainTime;

  //through num processes to execute them
  for (i = 0; i < NUM_PROCESSES; i++)
  {
    //initialize shortestRemainTime
    shortestRemainTime = -1;
    //find shortest remaining time
    for (j = 0; j < NUM_PROCESSES; j++)
    {
      //if shortest_remaining time hasn't been set, the
      //current process has arrived (arrival time <= system
      //time) then overwrite it, and the current process
      //hasn't finished
      if (shortestRemainTime < 0 && proc[j].arrivaltime <= sysTime && !proc[j].flag)
        shortestRemainTime = j;
      //else if shortest remaining time has been set, the
      //current process has arrived, the current process
      //runtime is shorter than shortest remaining time, and
      //current process hasn't finished than overwrite it
      else if (shortestRemainTime >= 0 && proc[j].arrivaltime <= sysTime && proc[j].runtime < proc[shortestRemainTime].runtime && !proc[j].flag)
        shortestRemainTime = j;
    }

    //if no process was found advance system time and continue
    if (shortestRemainTime < 0)
    {
      sysTime++;
      i--;
      continue;
    }

    //execute process
    //set shortestRemainTime start time
    proc[shortestRemainTime].starttime = sysTime;
    //advance system time
    sysTime += proc[shortestRemainTime].runtime;
    //set shortestRemainTime end time
    proc[shortestRemainTime].endtime = sysTime;
    //track of completion time running total
    totalComRunTime += (proc[shortestRemainTime].endtime - proc[shortestRemainTime].arrivaltime);
    //mark shortestRemainTime as completed
    proc[shortestRemainTime].flag = 1;

    printf("Process %d started at time %d\n", shortestRemainTime, proc[shortestRemainTime].starttime);
    printf("Process %d finished at time %d\n", shortestRemainTime, proc[shortestRemainTime].endtime);
  }

  //calculate average completion time
  avgComRunTime = totalComRunTime / NUM_PROCESSES;

  printf("Average time from arrival to completion is %d seconds\n", avgComRunTime);
}

void round_robin(struct process *proc)
{
  /* Implement scheduling algorithm here */

  //counters
  int i, j = 0;
  //which process id we searched first at current system time
  int startJ = 0;
  //running total of completion time
  int totalComRunTime = 0;
  //average completion time
  int avgComRunTime;
  //system time
  int sysTime = 0;
  //0 until a job completes
  int procFinish;
  //through till all processes have completed
  for (i = 0; i < NUM_PROCESSES; i++)
  {
    //initialize procFinish
    procFinish = 0;
    while (!procFinish)
    {
      //if proc[j] has arrived and has not completed then run it for 1 second
      if (proc[j].arrivaltime <= sysTime && proc[j].flag != 2)
      {
        //if proc[j] just started running initialize it
        if (!proc[j].flag)
        {
          proc[j].flag = 1; //process started
          proc[j].starttime = sysTime;
          proc[j].remainingtime = proc[j].runtime - 1;
        }

        //else update process
        else
        {
          proc[j].remainingtime--;
          //if proc is finished update proc
          if (!proc[j].remainingtime)
          {
            //process has completed
            proc[j].flag = 2;
            proc[j].endtime = sysTime + 1;
            procFinish = 1;
            totalComRunTime += (proc[j].endtime - proc[j].arrivaltime);
            printf("Process %d started at "
                   "time %d\n",
                   j, proc[j].starttime);
            printf("Process %d finished "
                   "at time %d\n",
                   j, proc[j].endtime);
          }
        }
        //update j and increment system time
        j = (j < (NUM_PROCESSES - 1)) ? (j + 1) : 0;
        sysTime++;
        startJ = j;
      }
      //if proc[j] can't be ran
      else
      {
        //update j
        j = (j < (NUM_PROCESSES - 1)) ? (j + 1) : 0;
        //if j = startJ then increment system time since no process could be run at this time
        if (j == startJ)
          sysTime++;
      }
    }
  }

  //calculate average completion time
  avgComRunTime = totalComRunTime / NUM_PROCESSES;

  printf("Average time from arrival to completion is %d seconds\n", avgComRunTime);
}

void round_robin_priority(struct process *proc)
{
  /* Implement scheduling algorithm here */
  //counters
  int i, j = 0;
  //which process id we searched first at current system time
  int startJ = 0;
  //running total of completion time
  int totalComRunTime = 0;
  //average completion time
  int avgComRunTime;
  //system time
  int sysTime = 0;
  //0 until a job completes
  int procFinish;
  //stores highest priority
  int greatPrior;
  //stores last executed process
  int lastExec = 0;
  //through till all processes have completed
  for (i = 0; i < NUM_PROCESSES; i++)
  {
    //initialize procFinish
    procFinish = 0;
    while (!procFinish)
    {
      //initialize greatPrior
      greatPrior = -1;
      do
      {
        //if proc[j] has arrived, is not done, and highest priority has not been set, set highest priority
        if (proc[j].arrivaltime <= sysTime && greatPrior < 0 && proc[j].flag < 2)
          greatPrior = j;

        //if proc[j] has arrived, is not done, and is of higher priority than highest priority, update greatPrior
        else if (proc[j].arrivaltime <= sysTime && proc[j].flag < 2 && proc[j].priority > proc[greatPrior].priority)
          greatPrior = j;
        j = (j < (NUM_PROCESSES - 1)) ? (j + 1) : 0;
      } while (j != startJ);

      //if highest_priority has been set execute
      if (greatPrior > -1)
      {
        lastExec = greatPrior;
        //if greatPrior just started running initialize it
        if (!proc[greatPrior].flag)
        {
          //process started
          proc[greatPrior].flag = 1;
          proc[greatPrior].starttime = sysTime;
          proc[greatPrior].remainingtime = proc[greatPrior].runtime - 1;
        }

        //else update process
        else
        {
          proc[greatPrior].remainingtime--;

          //if proc is finished, update proc
          if (!proc[greatPrior].remainingtime)
          {
            //process has completed
            proc[greatPrior].flag = 2;
            proc[greatPrior].endtime = sysTime + 1;
            procFinish = 1;
            totalComRunTime += (proc[greatPrior].endtime - proc[greatPrior].arrivaltime);
            printf("Process %d started at "
                   "time %d\n",
                   greatPrior, proc[greatPrior].starttime);
            printf("Process %d finished "
                   "at time %d\n",
                   greatPrior, proc[greatPrior].endtime);
          }
        }
      }
      //increment system time and set j to search from current highest priority job
      sysTime++;
      if (lastExec == NUM_PROCESSES - 1)
        startJ = j = 0;
      else
        startJ = j = (lastExec + 1);
    }
  }

  //calculate average completion time
  avgComRunTime = totalComRunTime / NUM_PROCESSES;

  printf("Average time from arrival to completion is %d seconds\n", avgComRunTime);
}
