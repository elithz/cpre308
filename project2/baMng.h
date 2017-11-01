/**
*		Filename:  baMng.h
*    Description:  Bank Account Manage Server headfile
*        Version:  1.0
*        Created:  10.23.2017 13h05min23s
*         Author:  Ningyuan Zhang （狮子劫博丽）(elithz), elithz@iastate.edu 
*        Company:  NERVE Software
*/

//define needed headfiles...
#ifndef BAMNG
#define BAMNG

#ifndef PTHREAD
#define PTHREAD
#include <pthread.h>
#endif

#ifndef BANK
#define BANK
#include "Bank.h"
#endif

#ifndef STDLIB
#define STDLIB
#include <stdlib.h>
#endif

#ifndef STRING
#define STRING
#include <string.h>
#endif

#ifndef STDIO
#define STDIO
#include <stdio.h>
#endif

#ifndef TIME
#define TIME
#include <sys/time.h>
#endif


//store a command within linked list
typedef struct LinkedCommand_struct{
	char * command;
	int id;
	struct timeval timestamp;
	struct LinkedCommand_struct * next;
	
}LinkedCommand;

//store a mutex lock associated with each bank account
typedef struct account_struct{
	pthread_mutex_t lock;
	int value;
}account;


//data about a linked list
typedef struct LinkedList_struct{
	pthread_mutex_t lock;
	LinkedCommand * head;
	LinkedCommand * tail;
	int size;
}LinkedList;

// //free memory allocated for bankAccount not used
// void freeAccount();

//get next command in linked list
LinkedCommand nextCmd();

//add command onto linked list
int addCmd(char * given_command, int id);

//lock account mutex
int lockAct(account * to_lock);

//unlock account mutex
int uLckAct(account * to_unlock);



#endif