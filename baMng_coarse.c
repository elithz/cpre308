/**
*		Filename:  baMng_coarse.c
*    Description:  Bank Account Manage Server
*        Version:  1.0
*        Created:  10.25.2017 16h00min01s
*         Author:  Ningyuan Zhang （狮子劫博丽）(elithz), elithz@iastate.edu 
*        Company:  NERVE Software
*/

#include "baMng.h"
#include <stdio.h>
#include <stdlib.h>

#define MAX_COMMAND_SIZE 200
#define NUM_ARGUMENTS 4
#define ARGUMENT_FORMAT "baMng workersNum accountNum out_file"

//function prototypes in bankAccountManager init and define prototypes
//parses command line arguments
int argParser(int argc, char** argv);
//sets up bank accounts
int accountSetup();
//sets up cmdBf
int cmdBufferSetup();
//client loop
int clientLoop();
//print incorrect argument format to stderr
void incorrectArgFmt();
//request handling worker threads
void * requestHdl();
//int to mark if the threads should be running or finishing up, give initial value 1
int running = 1;

//accounts
account * accounts;
//command buffer
LinkedList * cmdBf;
//workersNum and accounts
int workersNum;
int accountNum;
//lock to make str_tok threadsafe
pthread_mutex_t tokLk;
//bank lock
pthread_mutex_t bankLk;
//out file
FILE * outFPt;

/**initiate bank account manage server and take requests. will output
 * results to file provided at arv[3]
 * @param argv[1]: integer, number of working threads
 * @param argv[2]: integer, number of accounts
 * @param argv[3]: string, name of output file
 * @ret int: 0 = operation success, -1 = error encountered
 * @author elithz
 * @modified 10.25.2017*/
int main(int argc, char** argv){
	//initialize command buffer
	cmdBf = malloc(sizeof(LinkedList));

	//parse input arguments
	if(argParser(argc, argv))
		//argument error occured
		return -1;

	//initialize account space
	accounts = malloc(accountNum * sizeof(account));

	//set up bank accounts
	if(accountSetup())
		//error encountered during bank account setup
		return -1;

	//set up cmdBf
	if(cmdBufferSetup())
		//error encountered during command buffer setup
		return -1;

	//main command line loop
	if(clientLoop())
		//error encountered during client operations
		return -1;

	//free buffers
	free(cmdBf);
	free(accounts);
	// freeAccount();
	fclose(outFPt);

	return 0;
}

/**parse command line arguments
 * @ret int: 0 = operation success, -1 = operation failure
 * @author elithz
 * @modified 10.25.2017*/
int argParser(int argc, char** argv){
	//check for correct number of arguments
	if(argc != NUM_ARGUMENTS){
		fprintf(stderr, "error (baMng): incorrect # of command " 
			"line arguments\n");
		fprintf(stderr, "baMng expected format: ");
		fprintf(stderr, ARGUMENT_FORMAT);
		fprintf(stderr, "\nbaMng: exiting program\n");
		return -1;
	}

	//store arguments
	if(!sscanf(argv[1], "%d", &workersNum)){
		//sscanf failed to read an integer
		incorrectArgFmt();
		return -1;
	}

	if(!sscanf(argv[2], "%d", &accountNum)){
		//sscanf failed to read an integer
		incorrectArgFmt();
		return -1;
	}

	//open output file
	outFPt = fopen(argv[3], "w");

	if(!outFPt){
		/*failed to open file*/
		fprintf(stderr, "error (baMng): failed to open out_file\n");
		fprintf(stderr, "baMng: exiting program\n");
		return -1;
	}

	//return successfully
	return 0;
}

/**set up accounts
 * @ret int: 0 = operation success -1 = failure
 * @author elithz
 * @modified 10.25.2017*/
int accountSetup(){
	//counter
	int i;

	//initialize BANK_accounts
	initialize_accounts(accountNum);

	//loop through accountNum, create accounts for each
	for(i = 0; i < accountNum; i++){
		pthread_mutex_init(&(accounts[i].lock), NULL);
		accounts[i].value = 0;
	}

	return 0;
}

/**initialize command buffer
 * @ret int: 0 = operation success, -1 = failure
 * @author elithz
 * @modified 10.25.2017 */
int cmdBufferSetup(){
	//initialize command buffer
	pthread_mutex_init(&(cmdBf->lock), NULL);
	cmdBf->head = NULL;
	cmdBf->tail = NULL;
	cmdBf->size = 0;

	//return successfully
	return 0;
}

/**loops and executes commands as threads until the exit
 * function is called. After exit is called, the rest of the commands are ran
 * and the threads are joined before the function returns
 * @ret int: 0 = op success -1 = failure
 * @author elithz
 * @modified 10.25.2017 */
int clientLoop(){
	//pthread workers
	pthread_t workers[workersNum];
	//counter
	int i;
	//size_t used for getline
	size_t n = 100;
	//return value of getline
	ssize_t readSize = 0;
	//entered command
	char * command = malloc(MAX_COMMAND_SIZE * sizeof(char));
	//command ID
	int id = 1;

	//init workers
	for(i = 0; i < workersNum; i++)
		pthread_create(&workers[i], NULL, requestHdl, NULL);
		//pthread_create(&workers[i], NULL, (void*)&requestHdl, NULL);
	pthread_mutex_init(&tokLk, NULL);
	pthread_mutex_init(&bankLk, NULL);

	//client loop
	while(1){
		//read line
		readSize = getline(&command, &n, stdin);

		//null terminate line
		command[readSize - 1] = '\0';

		//check for end
		if(strcmp(command, "END") == 0){
			//mark done
			running = 0;
			//break loop and perform cleanup
			break;
		}

		//print command id
		printf("ID %d\n", id);

		//add command to buffer
		addCmd(command, id);

		//incremend id
		id++;
	}

	//clear out command buffer
	while(cmdBf->size > 0)
		//sleep to release control to worker thread
		usleep(1);

	//wait for workers to finish
	for(i = 0; i < workersNum; i++)
		pthread_join(workers[i], NULL);

	//free command
	free(command);

	//return successfully
	return 0;
}

/**print incorrect argument format to stderr
 * @ret void
 * @author elithz
 * @modified 10.25.2017*/
void incorrectArgFmt(){
	fprintf(stderr, "error(baMng): incorrect argument format\n");
	fprintf(stderr, "baMng expected format: ");
	fprintf(stderr, ARGUMENT_FORMAT);
	fprintf(stderr, "\nbaMng: exiting program\n");
}

void * requestHdl(){
	//current command
	LinkedCommand cmd;

	//string of arguments
	char ** cmdTk = malloc(21 * sizeof(char*));

	//current argument
	char * curTok;

	//number of arguments in command
	int tokenNum = 0;

	//stores account to check
	int check_account;

	//stores amount in check command
	int amount;

	//counters
	int i, j;

	//flag to mark insufficient funds
	int ISF = 0;

	//store timestamp
	struct timeval timestamp2;

	//while main loop is running or buffer isn't empty
	while(running || cmdBf->size > 0){
		//if no commands are present then sleep to release control of the thread
		while(cmdBf->size == 0 && running)
			usleep(1);

		//get next command
		cmd = nextCmd();

		if(!cmd.command)
			//if command is NULL we are currently out of commands
			continue;

		//parse command
		pthread_mutex_lock(&tokLk);
		curTok = strtok(cmd.command, " ");
		while(curTok){
			cmdTk[tokenNum] = malloc(21 * sizeof(char));
			strncpy(cmdTk[tokenNum], curTok, 21);
			tokenNum++;
			curTok = strtok(NULL, " ");
		}
		pthread_mutex_unlock(&tokLk);
		//execute command
		//is it a CHECK command?
		if(strcmp(cmdTk[0], "CHECK") == 0 && tokenNum == 2){
			pthread_mutex_lock(&tokLk);
			check_account = atoi(cmdTk[1]);
			pthread_mutex_unlock(&tokLk);
			pthread_mutex_lock(&bankLk);
			amount = read_account(check_account);
			pthread_mutex_unlock(&bankLk);
			gettimeofday(&timestamp2, NULL);
			flockfile(outFPt);
			fprintf(outFPt, "%d BAL %d TIME %d.%06d %d.%06d\n", 
				cmd.id, amount, cmd.timestamp.tv_sec, 
				cmd.timestamp.tv_usec, timestamp2.tv_sec, 
				timestamp2.tv_usec);
			funlockfile(outFPt);
		}
		//is it a TRANS command
		else if(strcmp(cmdTk[0], "TRANS") == 0 && tokenNum % 2 && tokenNum > 1){
			//variables to store transaction info
			int num_trans = (tokenNum - 1) / 2;
			int trans_accounts[num_trans];
			int trans_amounts[num_trans];
			int trans_balances[num_trans];
			int temp;

            //lock bank
			pthread_mutex_lock(&bankLk);

			//store accounts and transfer amounts
			for(i = 0; i < num_trans; i++){
				pthread_mutex_lock(&tokLk);
				trans_accounts[i] = atoi(cmdTk[i*2+1]);
				trans_amounts[i] = atoi(cmdTk[i*2+2]);
				pthread_mutex_unlock(&tokLk);
			}

			// //sort transactions in ascending order by account number
			// for(i = 0; i < num_trans; i++){
			// 	for(j = i; j < num_trans; j++){
			// 		if(trans_accounts[j] < trans_accounts[i]){
			// 			temp = trans_accounts[i];
			// 			trans_accounts[i] = trans_accounts[j];
			// 			trans_accounts[j] = temp;
			// 			temp = trans_amounts[i];
			// 			trans_amounts[i] = trans_amounts[j];
			// 			trans_amounts[j] = temp;
			// 		}
			// 	}
			// }

			// //lock accounts
			// for(i = 0; i < num_trans; i++)
			// 	pthread_mutex_lock(&(accounts[trans_accounts[i]-1].lock));

			// //check all transactions for sufficient funds
			// for(i = 0; i < num_trans; i++){
			// 	trans_balances[i] = read_account(trans_accounts[i]);
			// 	if(trans_balances[i] + trans_amounts[i] < 0){
			// 		//if ISF then let program know and * print to out file
			// 		gettimeofday(&timestamp2, NULL);
			// 		flockfile(outFPt);
			// 		fprintf(outFPt, "%d ISF %d TIME " 
			// 			"%d.06%d %d.06%d\n", 
			// 			cmd.id, trans_accounts[i], 
			// 			cmd.timestamp.tv_sec, 
			// 			cmd.timestamp.tv_usec, 
			// 			timestamp2.tv_sec, 
			// 			timestamp2.tv_usec);
			// 		funlockfile(outFPt);
			// 		ISF = 1;
			// 		break;
			// 	}
            // }
            

            //check all transactions for sufficient funds
			for(i = 0; i < num_trans; i++){
				trans_balances[i] = read_account(trans_accounts[i]);
				if(trans_balances[i] + trans_amounts[i] < 0){
					//if ISF then let program know and print to out file
					gettimeofday(&timestamp2, NULL);
					flockfile(out_fp);
					fprintf(out_fp, "%d ISF %d TIME " 
						"%d.06%d %d.06%d\n", 
						cmd.id, trans_accounts[i], 
						cmd.timestamp.tv_sec, 
						cmd.timestamp.tv_usec, 
						timestamp2.tv_sec, 
						timestamp2.tv_usec);
					funlockfile(out_fp);
					ISF = 1;
					break;
				}
			}


			//if we have sufficient funds
			if(!ISF){
				//execute transactions
				for(i = 0; i < num_trans; i++)
					write_account(trans_accounts[i], (trans_balances[i] + trans_amounts[i]));
				//print transaction success
				gettimeofday(&timestamp2, NULL);
				flockfile(outFPt);
				fprintf(outFPt, "%d OK TIME %d.%06d %d.%06d\n", 
					cmd.id, cmd.timestamp.tv_sec, 
					cmd.timestamp.tv_usec, 
					timestamp2.tv_sec, timestamp2.tv_usec);
				funlockfile(outFPt);
			}
			//unlock accounts
			// for(i = num_trans - 1; i >=0; i--)
            // 	pthread_mutex_unlock(&(accounts[trans_accounts[i]-1].lock));
            pthread_mutex_unlock(&bankLk);
		}
		//invalid command
		else
			fprintf(stderr, "%d INVALID REQUEST FORMAT\n", cmd.id);
		//free command
		free(cmd.command);
		for(i = 0; i < tokenNum; i++)
			free(cmdTk[i]);
		tokenNum = 0;
		ISF = 0;
	}

	free(cmdTk);

	//return
	return;
}

/**attempt to lock an account mutex
 * @param account * toLk: account structure to attempt to lock
 * @ret int: 0 = operation success; -1 = operation failure
 * @author elithz
 * @modified 10.25.2017*/
int lockAccount(account * toLk){
	if(pthread_mutex_trylock(&(toLk->lock)))
		//lock failed return -1
		return -1;

	//locked, return success
	return 0;
}

/**unlock an account mutex
 * @param account * to_unlock: account structure to attempt to unlock
 * @ret int: 0 = operation success; -1 = operation failure
 * @author elithz
 * @modified 10.25.2017*/
int unlockAccount(account * to_unlock){
	//unlock account
	pthread_mutex_unlock(&(to_unlock->lock));

	//return successfully
	return 0;
}

/**get next element in linked list
 * @param LinkedList * command_buffer: linked list to pull from
 * @ret char * command: will be next command in LinkedList (NULL if no command 
 * exists)
 * @author elithz
 * @modified 10.25.2017*/
LinkedCommand nextCmd(){
	//temporary pointer used to free head
	LinkedCommand * temp_head;

	//initialize return value
	LinkedCommand ret;

	//lock command buffer
	pthread_mutex_lock(&(cmdBf->lock));

	//are there any commands to pull?
	if(cmdBf->size > 0){
		//set return value
		ret.id = cmdBf->head->id;
		ret.command = malloc(MAX_COMMAND_SIZE * sizeof(char));
		ret.timestamp = cmdBf->head->timestamp;
		strncpy(ret.command, cmdBf->head->command, 
			MAX_COMMAND_SIZE);
		ret.next = NULL;

		//move head and free memory from linked list
		temp_head = cmdBf->head;
		cmdBf->head = cmdBf->head->next;
		free(temp_head->command);
		free(temp_head);

		//if head ran past tail set tail back to null
		if(!cmdBf->head)
			cmdBf->tail = NULL;

		//update linked list size
		cmdBf->size = cmdBf->size - 1;
	}
	else{
		//unlock command buffer
		pthread_mutex_unlock(&(cmdBf->lock));

		ret.command = NULL;

		//no commands in buffer
		return ret;
	}

	//unlock command buffer
	pthread_mutex_unlock(&(cmdBf->lock));

	//return command
	return ret;
}

/**add command onto linked list
 * @param LinkedList * command_buffer: command buffer to add command onto
 * @ret int: 0 = operation success -1 = operation failure
 * @author elithz
 * @modified 10.25.2017*/
int addCmd(char * given_command, int id){
	//initialize new LinkedCommand to add to list
	LinkedCommand * new_tail = malloc(sizeof(LinkedCommand));

	//construct the command
	new_tail->command = malloc(MAX_COMMAND_SIZE * sizeof(char));
	strncpy(new_tail->command, given_command, MAX_COMMAND_SIZE);
	new_tail->id = id;
	gettimeofday(&(new_tail->timestamp), NULL);
	new_tail->next = NULL;

	//lock command_buffer
	pthread_mutex_lock(&(cmdBf->lock));

	//is the list currently empty
	if(cmdBf->size > 0){
		//have tail point to this new command
		cmdBf->tail->next = new_tail;

		//set new tail and increment size of linked list
		cmdBf->tail = new_tail;
		cmdBf->size = cmdBf->size + 1;
	}
	else{
		//add first element and set head and tail to it
		cmdBf->head = new_tail;
		cmdBf->tail = new_tail;

		//size is one
		cmdBf->size = 1;
	}

	//unlock command buffer
	pthread_mutex_unlock(&(cmdBf->lock));

	//return successfully
	return 0;
}


// /*
//  * frees memory allocated for Bank Accounts
//  */
// void freeAccount()
// {
// 	free(BANK_accounts);
// }