/**
*		Filename:  baMng.h
*    Description:  Bank Account Manage Server headfile
*        Version:  1.0
*        Created:  10.23.2017 13h05min23s
*         Author:  Ningyuan Zhang （狮子劫博丽）(elithz), elithz@iastate.edu 
*        Company:  NERVE Software
*/
#ifndef BAMNG
#define BAMNG

#ifndef STDIO
#define STDIO
#include <stdio.h>
#endif

#ifndef STDLIB
#define STDLIB
#include <stdlib.h>
#endif

#ifndef STRING
#define STRING
#include <string.h>
#endif

#ifndef PTHREAD
#define PTHREAD
#include <pthread.h>
#endif

#ifndef BANK
#define BANK
#include "Bank.h"
#endif

#ifndef TIME
#define TIME
#include <sys/time.h>
#endif

//structure used to store a mutex lock associated with each bank account
typedef struct account_struct{
	pthread_mutex_t lock;
	int value;
}account;

//structure used to store a command within linked list
typedef struct LinkedCommand_struct{
	char * command;
	int id;
	struct timeval timestamp;
	struct LinkedCommand_struct * next;
	
}LinkedCommand;

//stucture used to store data about a linked list
typedef struct LinkedList_struct{
	pthread_mutex_t lock;
	LinkedCommand * head;
	LinkedCommand * tail;
	int size;
}LinkedList;

// //frees memory allocated for Bank Accounts
// void freeAccount();

//function to attempt to lock an account mutex
int lockAccount(account * to_lock);

//function to unlock an account mutex
int unlockAccount(account * to_unlock);

//function to get next command in linked list
LinkedCommand nextCmd();

//function used to add command onto linked list
int addCmd(char * given_command, int id);

#endif