#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#define MICRO_SEC 100000

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
double overall_waiting_time;
double over_all_bclass_wait;
double over_all_eclass_wait;
int MAX_LENGTH = 1024;
int queue_length[];

Queue *bus_queue;
Queue *eco_queue;
customerNode cusList[2000];

pthread_t customers_thread[1024];
pthread_t clerks_thread[5];

//access queue mutex
pthread_cond_t queue_convar = PTHREAD_COND_INITIALIZER;
pthread_mutex_t queue_mutex= PTHREAD_MUTEX_INITIALIZER;

//convar and mutex for clerk
pthread_mutex_t Clerk_mutex1= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t Clerk_mutex2= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t Clerk_mutex3= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t Clerk_mutex4= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t Clerk_mutex5= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t Clerk_convar1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t Clerk_convar2 = PTHREAD_COND_INITIALIZER;
pthread_cond_t Clerk_convar3 = PTHREAD_COND_INITIALIZER;
pthread_cond_t Clerk_convar4 = PTHREAD_COND_INITIALIZER;
pthread_cond_t Clerk_convar5 = PTHREAD_COND_INITIALIZER;

 //convar and mutex for queue
pthread_mutex_t bus_qm= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t eco_qm= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t bus_qc = PTHREAD_COND_INITIALIZER;
pthread_cond_t eco_qc = PTHREAD_COND_INITIALIZER;

int clerk[5]={0,0,0,0,0};
int eco_cus_size = 0;
int bus_cus_size = 0;
struct timeval init_time;


int check_clerk_avaliable(){
	for(int i = 0; i < 5 ; i++){
		if(clerk[i]==0){
			
			return i;//this clerk avaliable
		}
	}
	return -1;
}


//get the time difference
double calculate_time(){
	struct timeval present_time;
	gettimeofday(&present_time,NULL);
	double start_in_microsec = (init_time.tv_sec*10*MICRO_SEC)+init_time.tv_usec;
	double present_in_microsec = (present_time.tv_sec*10*MICRO_SEC)+present_time.tv_usec;
	return (double)(present_in_microsec-start_in_microsec)/(10*MICRO_SEC);

}


void * customer_thread(void * cus_info){
	
	struct customerNode * p_myInfo = (customerNode *) cus_info;
	
	usleep(p_myInfo->arrival_t*MICRO_SEC);
	
	printf("A customer arrives: customer ID %2d. \n", p_myInfo->id);
	
	//economy class customer
	if(p_myInfo->flight_class==0){
		eco_cus_size++;
		int clerkID = check_clerk_avaliable();
		pthread_mutex_lock(&eco_qm);
		{
			
			enqueue(eco_queue,p_myInfo);
			printf("A customer enters a queue: the queue ID %1d, and length of the queue %2d. \n", 0, eco_queue->q_size);
			
			
			while (clerkID==-1) {
				pthread_cond_wait(&eco_qc,&eco_qm);
			}
			
	
			dequeue(eco_queue, p_myInfo);
		}
		pthread_mutex_unlock(&eco_qm);
		clerk[clerkID] = 1;
		usleep(10);

		p_myInfo->start_t = calculate_time();
		printf("A clerk starts serving a customer: start time %.2f, the customer ID %2d, the clerk ID %1d. \n", p_myInfo->start_t,p_myInfo->id,clerkID);

		usleep(p_myInfo->duration * MICRO_SEC);
		
		p_myInfo->end_t = calculate_time();
		//record the wait time 
		over_all_bclass_wait+= p_myInfo->end_t-p_myInfo->start_t;
		overall_waiting_time+=p_myInfo->end_t-p_myInfo->start_t;
		printf("A clerk finishes serving a customer: end time %.2f, the customer ID %2d, the clerk ID %1d. \n", p_myInfo->end_t,p_myInfo->id,clerkID);
		
		if(clerkID==0){
			pthread_mutex_signal(&Clerk_convar1);
		}
		else if(clerkID==1){
			pthread_mutex_signal(&Clerk_convar2);
		}
		else if(clerkID==2){
			pthread_mutex_signal(&Clerk_convar3);
		}
		else if(clerkID==3){
			pthread_mutex_signal(&Clerk_convar4);
		}
		else if(clerkID==4){
			pthread_mutex_signal(&Clerk_convar5);
		}
		pthread_exit(NULL);
	
		return NULL;
	}
	//bussiness class customer
	else{
		bus_cus_size++;
		int clerkID = check_clerk_avaliable();
		pthread_mutex_lock(&bus_qm);
		{
			enqueue(bus_queue,p_myInfo);
			printf("A customer enters a queue: the queue ID %1d, and length of the queue %2d. \n", 1,bus_queue->q_size);
			
			while (check_clerk_avaliable()==-1) {
				pthread_cond_wait(&bus_qc,&bus_qm);
			}
			dequeue(bus_queue, p_myInfo);
		}
		pthread_mutex_unlock(&bus_qm);
		clerk[clerkID]=1;
		usleep(10);

		p_myInfo->start_t =calculate_time();
		printf("A clerk starts serving a customer: start time %.2f, the customer ID %2d, the clerk ID %1d. \n", p_myInfo->start_t,p_myInfo->id,clerkID);
		usleep(p_myInfo->duration * MICRO_SEC);

		p_myInfo->end_t = calculate_time();
		//record the wait time
		over_all_bclass_wait+= p_myInfo->end_t-p_myInfo->start_t;
		overall_waiting_time+=p_myInfo->end_t-p_myInfo->start_t;

		printf("A clerk finishes serving a customer: end time %.2f, the customer ID %2d, the clerk ID %1d. \n", p_myInfo->end_t,p_myInfo->id,clerkID);

		if(clerkID==0){
			pthread_mutex_signal(&Clerk_convar1);
		}
		else if(clerkID==1){
			pthread_mutex_signal(&Clerk_convar2);
		}
		else if(clerkID==2){
			pthread_mutex_signal(&Clerk_convar3);
		}
		else if(clerkID==3){
			pthread_mutex_signal(&Clerk_convar4);
		}
		else if(clerkID==4){
			pthread_mutex_signal(&Clerk_convar5);
		}
		pthread_exit(NULL);
	
		return NULL;
	}
	

		
		
			
	}
	

// function entry for clerk threads
void *clerk_thread(void * clerkNum){
	int clerk_idle;
	int clerkID = clerkNum;
	while(1){
		
		/* selected_queue_ID = Select the queue based on the priority and current customers number */
		
		pthread_mutex_lock(&queue_mutex);
		//if no one is waiting
		while(bus_queue->q_size==0 || eco_queue->q_size==0){
			pthread_cond_wait(&queue_convar,&queue_mutex);
		}
		pthread_mutex_unlock(&queue_mutex);
		//do the business class first
		
		if(bus_queue->q_size!=0){
			//mark the clerk is idle
			//clerk_idle = check_clerk_avaliable();
			if(clerkID!=-1){
				clerk[clerkID]=1;
			}
			if(pthread_cond_signal(&bus_qc)){
				perror("condition signal failed");
				exit(0);
			}


			//unlock mutex
			pthread_mutex_unlock(&bus_qm);
			if(clerkID==0){
				pthread_mutex_lock(&Clerk_mutex1);
				pthread_cond_wait(&Clerk_convar1,&Clerk_mutex1);
				pthread_mutex_unlock(&Clerk_mutex1);
			}
			else if(clerkID==1){
				pthread_mutex_lock(&Clerk_mutex2);
				pthread_cond_wait(&Clerk_convar2,&Clerk_mutex2);
				pthread_mutex_unlock(&Clerk_mutex2);
			}
			else if(clerkID==2){
				pthread_mutex_lock(&Clerk_mutex3);
				pthread_cond_wait(&Clerk_convar3,&Clerk_mutex3);
				pthread_mutex_unlock(&Clerk_mutex3);
			}
			else if(clerkID==3){
				pthread_mutex_lock(&Clerk_mutex4);
				pthread_cond_wait(&Clerk_convar4,&Clerk_mutex4);
				pthread_mutex_unlock(&Clerk_mutex4);
			}
			else if(clerkID==4){
				pthread_mutex_lock(&Clerk_mutex5);
				pthread_cond_wait(&Clerk_convar5,&Clerk_mutex5);
				pthread_mutex_unlock(&Clerk_mutex5);
			}

		}
	}
	
	pthread_exit(NULL);
	
	return NULL;
}



int main(int argc,char** argv) {
	FILE * fp;
	
	//char str[24];
	int NCustomers, cus_index=0;
	int ID, class_type, start_t, duration;
	fp = fopen(argv[1], "r+");
	//exit if file DNE
	if(fp ==NULL){
		exit(0);
 	}
	rewind(fp);
	//get the first line of file check total customer size
	fscanf(fp, "%d", &NCustomers);
	printf("total: %d\n", NCustomers);
	
	
	
	
	while(cus_index < NCustomers){
		
		fscanf(fp,"%d:%d,%d,%d",&ID,&class_type,&start_t,&duration);
		//printf("%d, %d, %d, %d\n\n", ID, class_type, start_t,duration);
		customerNode customer;
		customer.id = ID;
		customer.flight_class = class_type;
		customer.arrival_t = start_t;
		customer.duration = duration;
		cusList[cus_index] = customer;
		// if(class_type == 1){
		// 	cus->id=ID;
		// 	cus->flight_class = 1;
		// 	cus->arrival_t = start_t;
		// 	cus->duration = duration;
		// 	enqueue(bus_queue,cus);
		// }
		// else{
		// 	cus->id=ID;
		// 	cus->flight_class = 0;
		// 	cus->arrival_t = start_t;
		// 	cus->duration = duration;
		// 	enqueue(eco_queue,cus);		
		// }
		cus_index++;
	}

	for(int i = 0; i < 5; i++){ // number of clerks
		pthread_create(&clerks_thread[i], NULL, clerk_thread, (void *)&clerk[i]); // clerk_info: passing the clerk information (e.g., clerk ID) to clerk thread
	}

	//create customer thread
	for(int i = 0; i < NCustomers; i++){ // number of customers
		pthread_create(&customers_thread[i], NULL, customer_thread, (void *)&cusList[i]); //custom_info: passing the customer information (e.g., customer ID, arrival time, service time, etc.) to customer thread
	}
	for(int i = 0; i < NCustomers; i++ ){
		if(pthread_join(customers_thread[i],NULL)!=0){
			perror("Fail to join the threads");
			exit(0);
		}
	}
	
	// destroy mutex & condition variable (optional)
	pthread_mutex_destroy(&queue_mutex);
	pthread_cond_destroy(&queue_convar);

	pthread_mutex_destroy(&Clerk_mutex1);
	pthread_mutex_destroy(&Clerk_mutex2);
	pthread_mutex_destroy(&Clerk_mutex3);
	pthread_mutex_destroy(&Clerk_mutex4);
	pthread_mutex_destroy(&Clerk_mutex5);
	pthread_cond_destroy(&Clerk_convar1);
	pthread_cond_destroy(&Clerk_convar2);
	pthread_cond_destroy(&Clerk_convar3);
	pthread_cond_destroy(&Clerk_convar4);
	pthread_cond_destroy(&Clerk_convar5);

	pthread_mutex_destroy(&bus_qm);
	pthread_mutex_destroy(&eco_qm);
	pthread_cond_destroy(&bus_qc);
	pthread_cond_destroy(&eco_qc);

	
	// calculate the average waiting time of all customers

	fclose(fp);
	
	printf("The average waiting time for all customers in the system is: %.2f seconds. \n",overall_waiting_time);
	printf("The average waiting time for all business-class customers is: %.2f seconds. \n",over_all_bclass_wait);
	printf("The average waiting time for all economy-class customers is: %.2f seconds. \n",over_all_eclass_wait);

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