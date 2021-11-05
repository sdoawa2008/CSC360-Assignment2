#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#define MICRO_SEC 100000// 1
#define MAX_LENGTH 1024

typedef struct Queue{
	struct customerNode* head;
	struct customerNode* tail;
	int q_size;
} Queue;

typedef struct customerNode {
	int id;
	int flight_class;
	double start_t;
	double end_t;
	int arrival_t;
	int duration;
	struct customerNode* next;
} customerNode;

void enqueue(Queue* queue, customerNode* node);
void dequeue(Queue* queue, customerNode* node);

// Global variables
double overall_waiting_time = 0.0;
double over_all_bclass_wait = 0.0;
double over_all_eclass_wait = 0.0;

Queue bus_queue[MAX_LENGTH];
Queue eco_queue[MAX_LENGTH];
customerNode cusList[MAX_LENGTH*2];

pthread_t customers_thread[MAX_LENGTH];
pthread_t clerks_thread[5];

//access queue mutex
pthread_cond_t queue_convar = PTHREAD_COND_INITIALIZER;
pthread_mutex_t queue_mutex= PTHREAD_MUTEX_INITIALIZER;

//convar and mutex for clerk
pthread_mutex_t Clerk_mutex= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t Clerk_convar1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t Clerk_convar2 = PTHREAD_COND_INITIALIZER;
pthread_cond_t Clerk_convar3 = PTHREAD_COND_INITIALIZER;
pthread_cond_t Clerk_convar4 = PTHREAD_COND_INITIALIZER;
pthread_cond_t Clerk_convar5 = PTHREAD_COND_INITIALIZER;

//convar and mutex for differnt queues
pthread_mutex_t bus_qm= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t eco_qm= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t bus_qc = PTHREAD_COND_INITIALIZER;
pthread_cond_t eco_qc = PTHREAD_COND_INITIALIZER;

int clerk[5]={0,0,0,0,0}; // clerk id from 0-4
int eco_cus_size = 0; // total # of economy customer
int bus_cus_size = 0; // total # of business customer
struct timeval init_time;

// check if there's any clerk available among the total 5 clerks
int check_clerk_avaliable(){
	for(int i = 0; i < 5 ; i++){
		if(clerk[i] == 0){
			return i; // return the id of idle clerk
		}
	}
	return -1; // all clerks are occupied
}

//get the time difference
double calculate_time(){
	struct timeval present_time;
	gettimeofday(&present_time,NULL);
	double init_in_microsec = (init_time.tv_sec*10*MICRO_SEC) + init_time.tv_usec;
	double present_in_microsec = (present_time.tv_sec*10*MICRO_SEC) + present_time.tv_usec;
	// return value in seconds
	return (double)(present_in_microsec - init_in_microsec)/(10*MICRO_SEC);
}

void * customer_thread(void * cus_info){
	
	struct customerNode *p_myInfo = (customerNode*) cus_info;

	usleep(p_myInfo->arrival_t*MICRO_SEC);
	
	printf("A customer arrives: customer ID %2d. \n", p_myInfo->id);
	
	//economy class customers
	if(p_myInfo->flight_class == 0){
		eco_cus_size++;
		int clerkID = check_clerk_avaliable();
		pthread_mutex_lock(&eco_qm);
		{	
			enqueue(eco_queue,p_myInfo);
			printf("A customer enters a queue: the queue ID %1d, and length of the queue %2d. \n", 0, eco_queue->q_size);
			
			//  The customer will wait until they are at head of the queue and there's clerk available
			while (clerkID == -1) {
				pthread_cond_wait(&eco_qc, &eco_qm);
			}	
			dequeue(eco_queue, p_myInfo);
            clerk[clerkID] = 0; // mark the clerk as idle
		}

		pthread_mutex_unlock(&eco_qm);
		usleep(10);

        // get the init_time
		p_myInfo->start_t = calculate_time();
		printf("A clerk starts serving a customer: start time %.2f, the customer ID %2d, the clerk ID %1d. \n", p_myInfo->start_t, p_myInfo->id, clerkID);

		usleep(p_myInfo->duration * MICRO_SEC); // duration = 10*sec
		
		p_myInfo->end_t = calculate_time();

		//record the wait time 
		over_all_bclass_wait+= p_myInfo->end_t-p_myInfo->start_t;
		overall_waiting_time+= p_myInfo->end_t-p_myInfo->start_t;

		printf("A clerk finishes serving a customer: end time %.2f, the customer ID %2d, the clerk ID %1d. \n", p_myInfo->end_t, p_myInfo->id, clerkID);

		// inform clerk 
		if(clerkID==0){
            pthread_mutex_lock(&Clerk_mutex);
			pthread_cond_signal(&Clerk_convar1);
            pthread_mutex_unlock(&Clerk_mutex);
		}
		else if(clerkID==1){
            pthread_mutex_lock(&Clerk_mutex);
			pthread_cond_signal(&Clerk_convar2);
            pthread_mutex_unlock(&Clerk_mutex);
		}
		else if(clerkID==2){
            pthread_mutex_lock(&Clerk_mutex);
			pthread_cond_signal(&Clerk_convar3);
            pthread_mutex_unlock(&Clerk_mutex);
		}
		else if(clerkID==3){
            pthread_mutex_lock(&Clerk_mutex);
			pthread_cond_signal(&Clerk_convar4);
            pthread_mutex_unlock(&Clerk_mutex);
		}
		else if(clerkID==4){
            pthread_mutex_lock(&Clerk_mutex);
			pthread_cond_signal(&Clerk_convar5);
            pthread_mutex_unlock(&Clerk_mutex);
		}
	}
	//bussiness class customer
	else{
		bus_cus_size++;
		int clerkID = check_clerk_avaliable();
		pthread_mutex_lock(&bus_qm);
		{
			enqueue(bus_queue,p_myInfo);
			printf("A customer enters a queue: the queue ID %1d, and length of the queue %2d. \n", 1,bus_queue->q_size);
			while (clerkID == -1) {
				pthread_cond_wait(&bus_qc,&bus_qm);
			}
			dequeue(bus_queue, p_myInfo);
            clerk[clerkID] = 0; // mark the clerk as idle
		}
		pthread_mutex_unlock(&bus_qm);
		usleep(10);

		p_myInfo->start_t = calculate_time();
		printf("A clerk starts serving a customer: start time %.2f, the customer ID %2d, the clerk ID %1d. \n", p_myInfo->start_t,p_myInfo->id,clerkID);
		
        usleep(p_myInfo->duration * MICRO_SEC);

		p_myInfo->end_t = calculate_time();

		//record the wait time
		over_all_eclass_wait+= p_myInfo->end_t-p_myInfo->start_t;
		overall_waiting_time+=p_myInfo->end_t-p_myInfo->start_t;

		printf("A clerk finishes serving a customer: end time %.2f, the customer ID %2d, the clerk ID %1d. \n", p_myInfo->end_t, p_myInfo->id, clerkID);

        // inform clerk
		if(clerkID==0){
            pthread_mutex_lock(&Clerk_mutex);
			pthread_cond_signal(&Clerk_convar1);
            pthread_mutex_unlock(&Clerk_mutex);
		}
		else if(clerkID==1){
            pthread_mutex_lock(&Clerk_mutex);
			pthread_cond_signal(&Clerk_convar2);
            pthread_mutex_unlock(&Clerk_mutex);
		}
		else if(clerkID==2){
            pthread_mutex_lock(&Clerk_mutex);
			pthread_cond_signal(&Clerk_convar3);
            pthread_mutex_unlock(&Clerk_mutex);
		}
		else if(clerkID==3){
            pthread_mutex_lock(&Clerk_mutex);
			pthread_cond_signal(&Clerk_convar4);
            pthread_mutex_unlock(&Clerk_mutex);
		}
		else if(clerkID==4){
            pthread_mutex_lock(&Clerk_mutex);
			pthread_cond_signal(&Clerk_convar5);
            pthread_mutex_unlock(&Clerk_mutex);
		}
	
	}	

    pthread_exit(NULL);
	return NULL;
}
	

// function entry for clerk threads
void *clerk_thread(void * clerkNum){
	//int clerk_idle;
	int clerkID = (int)clerkNum;
	while(1){	
		pthread_mutex_lock(&queue_mutex);
		//if no one is waiting
		while(bus_queue->q_size==0 || eco_queue->q_size==0){
			pthread_cond_wait(&queue_convar,&queue_mutex);
		}
		pthread_mutex_unlock(&queue_mutex);

		//do the business class first
		if(bus_queue->q_size != 0){
			// // if the clerk is idle, mark it as occupied
			if(clerk[clerkID] != 1){
				clerk[clerkID] = 1;
			}
			 if (pthread_mutex_lock(&bus_qm) != 0)
    		{
                perror("mutex lock failed\n");
                exit(0);
            } 
			if(pthread_cond_signal(&bus_qc)){
				perror("condition signal failed\n");
				exit(0);
			}

			//unlock mutex
			pthread_mutex_unlock(&bus_qm);

			if(clerkID==0){
				pthread_mutex_lock(&Clerk_mutex);
				pthread_cond_wait(&Clerk_convar1,&Clerk_mutex);
				pthread_mutex_unlock(&Clerk_mutex);
			}
			else if(clerkID==1){
				pthread_mutex_lock(&Clerk_mutex);
				pthread_cond_wait(&Clerk_convar2,&Clerk_mutex);
				pthread_mutex_unlock(&Clerk_mutex);
			}
			else if(clerkID==2){
				pthread_mutex_lock(&Clerk_mutex);
				pthread_cond_wait(&Clerk_convar3,&Clerk_mutex);
				pthread_mutex_unlock(&Clerk_mutex);
			}
			else if(clerkID==3){
				pthread_mutex_lock(&Clerk_mutex);
				pthread_cond_wait(&Clerk_convar4,&Clerk_mutex);
				pthread_mutex_unlock(&Clerk_mutex);
			}
			else if(clerkID==4){
				pthread_mutex_lock(&Clerk_mutex);
				pthread_cond_wait(&Clerk_convar5,&Clerk_mutex);
				pthread_mutex_unlock(&Clerk_mutex);
			}
			
		}	
		//for economy queue customer
		else if(eco_queue->q_size!=0){
            
			if(clerk[clerkID] != 1){
				clerk[clerkID] = 1;
			}

             if (pthread_mutex_lock(&eco_qm) != 0)
    		{
                perror(" mutex lock failed\n");
                exit(0);
    		}
		
			if(pthread_cond_signal(&eco_qc)){
				perror("condition signal failed\n");
				exit(0);
            }

			//unlock mutex
			pthread_mutex_unlock(&eco_qm);

			if(clerkID==0){
				pthread_mutex_lock(&Clerk_mutex);
				pthread_cond_wait(&Clerk_convar1,&Clerk_mutex);
				pthread_mutex_unlock(&Clerk_mutex);
			}
			else if(clerkID==1){
				pthread_mutex_lock(&Clerk_mutex);
				pthread_cond_wait(&Clerk_convar2,&Clerk_mutex);
				pthread_mutex_unlock(&Clerk_mutex);
			}
			else if(clerkID==2){
				pthread_mutex_lock(&Clerk_mutex);
				pthread_cond_wait(&Clerk_convar3,&Clerk_mutex);
				pthread_mutex_unlock(&Clerk_mutex);
			}
			else if(clerkID==3){
				pthread_mutex_lock(&Clerk_mutex);
				pthread_cond_wait(&Clerk_convar4,&Clerk_mutex);
				pthread_mutex_unlock(&Clerk_mutex);
			}
			else if(clerkID==4){
				pthread_mutex_lock(&Clerk_mutex);
				pthread_cond_wait(&Clerk_convar5,&Clerk_mutex);
				pthread_mutex_unlock(&Clerk_mutex);
			}
		}
	}

	pthread_exit(NULL);
	return NULL;
}



int main(int argc,char** argv) {
	FILE * fp;
	int NCustomers, cus_index=0;
	int ID, class_type, start_t, duration;
	fp = fopen(argv[1], "r+");

	if(fp ==NULL){
		exit(0); 	//exit if file DNE
 	}

	rewind(fp);
	//get the first line of file check total customer size
	fscanf(fp, "%d", &NCustomers);
	// printf("total: %d\n", NCustomers);
	
	while(cus_index < NCustomers){
		fscanf(fp,"%d:%d,%d,%d",&ID,&class_type,&start_t,&duration);
		// printf("%d, %d, %d, %d\n\n", ID, class_type, start_t,duration);
		customerNode customer;
		customer.id = ID;
		customer.flight_class = class_type;
		customer.arrival_t = start_t;
		customer.duration = duration;
		cusList[cus_index] = customer;
		cus_index++;
	}

    fclose(fp);
	
    // initialize the mutex and convar
    if (pthread_cond_init(&queue_convar, NULL) != 0) {
        perror("pthread_cond_init() error");                                        
        exit(1);                                                                    
    }
    if (pthread_cond_init(&Clerk_convar1, NULL) != 0) {
        perror("pthread_cond_init() error");                                        
        exit(1);                                                                    
    }
    if (pthread_cond_init(&Clerk_convar2, NULL) != 0) {
        perror("pthread_cond_init() error");                                        
        exit(1);                                                                    
    }
    if (pthread_cond_init(&Clerk_convar3, NULL) != 0) {
        perror("pthread_cond_init() error");                                        
        exit(1);                                                                    
    }
    if (pthread_cond_init(&Clerk_convar4, NULL) != 0) {
        perror("pthread_cond_init() error");                                        
        exit(1);                                                                    
    }
    if (pthread_cond_init(&Clerk_convar5, NULL) != 0) {
        perror("pthread_cond_init() error");                                        
        exit(1);                                                                    
    }

    pthread_mutex_init(&queue_mutex, NULL);
    pthread_mutex_init(&Clerk_mutex, NULL);
    pthread_mutex_init(&Clerk_mutex, NULL);
    pthread_mutex_init(&Clerk_mutex, NULL);
    pthread_mutex_init(&Clerk_mutex, NULL);
    pthread_mutex_init(&Clerk_mutex, NULL);
    pthread_mutex_init(&bus_qm, NULL);
    pthread_mutex_init(&eco_qm, NULL);

	gettimeofday(&init_time,NULL);

	for(int i = 0; i < 5; i++){
		pthread_create(&clerks_thread[i], NULL, clerk_thread, (void *)&clerk[i]); 
	}

	//create customer thread
	for(int i = 0; i < NCustomers; i++){
		pthread_create(&customers_thread[i], NULL, customer_thread, (void *)&cusList[i]);
	}

	for(int i = 0; i < NCustomers; i++ ){
		usleep(10);
		if(pthread_join(customers_thread[i],NULL)!=0){
			perror("Fail to join customer threads");
			exit(0);
		}
	}
	
	// calculate the average waiting time of all customers
	printf("\nThe average waiting time for all customers in the system is: %.2f seconds. \n",overall_waiting_time/NCustomers);
	printf("The average waiting time for all business-class customers is: %.2f seconds. \n",over_all_bclass_wait/bus_cus_size);
	printf("The average waiting time for all economy-class customers is: %.2f seconds. \n",over_all_eclass_wait/eco_cus_size);

	// destroy mutex & condition variable (optional)
	// pthread_mutex_destroy(&queue_mutex);
	// pthread_cond_destroy(&queue_convar);

	// pthread_mutex_destroy(&Clerk_mutex);
	// pthread_cond_destroy(&Clerk_convar1);
	// pthread_cond_destroy(&Clerk_convar2);
	// pthread_cond_destroy(&Clerk_convar3);
	// pthread_cond_destroy(&Clerk_convar4);
	// pthread_cond_destroy(&Clerk_convar5);

	// pthread_mutex_destroy(&bus_qm);
	// pthread_mutex_destroy(&eco_qm);
	// pthread_cond_destroy(&bus_qc);
	// pthread_cond_destroy(&eco_qc);

	printf("\n\t***************************END*******************************\t\n");
	return 0;
}


void enqueue(Queue* queue, customerNode* node){
	if(queue->q_size==0){
		queue->head = node;
		queue->tail = node;
	}
	else{
		queue->tail->next = node;
		queue->tail = node;
	}
	queue->q_size++;
}

void dequeue(Queue* queue, customerNode* node){
	if(queue->q_size ==0){
		return;
	}
	if(queue->q_size==1){
		queue->head = NULL;
		queue->tail = NULL;
	}
	else{
		queue->head =queue->head->next;

	}
	queue->q_size--;
}