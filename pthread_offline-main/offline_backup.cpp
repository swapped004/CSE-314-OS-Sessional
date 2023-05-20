#include<cstdio>
#include<pthread.h>
#include<semaphore.h>
#include<queue>
#include <unistd.h>
#include<cmath>
#include<bits/stdc++.h>
#include<stdlib.h>
#include<iostream>
#include<random>
#include<time.h>

using namespace std;

typedef long long ll;


//input parameters taken from file
int M,N,P,w,x,y,z;

//keep track of time
time_t start,ending;

//semaphore to control sleep and wake up

//printing semaphore -> console is a shared resource as well!!!
pthread_mutex_t mut;
//kiosk check-in semaphore
pthread_mutex_t kiosk_update;
sem_t kiosk;
//security check semaphores
vector<sem_t> security_sem;
//boarding semaphore
sem_t boarding_gate;
//for accessing priority queue
pthread_mutex_t prior_mutex;
sem_t prior_empty;
//VIP gate semaphores
sem_t left_right_sem;
sem_t right_left_sem;
sem_t gate_sync_sem;
//special kiosk sem
sem_t special_kiosk_sem;



//first allocated kiosk is no.1
int kiosk_no = 1;

//left_right and right_left counts
int left_right_passengers = 0;
int right_left_passengers = 0;


//structure to multiple pass values to thread routine functions
struct info{
	int pass_id;
	bool vip_staus;
	int thread_priority;
	int timer;
};

//comparator function for priority_queue (Max queue based on thread priority)
struct CompareHeight {
    bool operator()(pair<int,info> const& p1, pair<int,info> const& p2)
    {
        // Max priority queue
        return p1.first < p2.first;
    }
};

//priority queue for thread scheduling
priority_queue<pair<int, info>, vector<pair<int, info>>,CompareHeight> prior_queue;


//functions
void* boarding_func(void *arg);
void* vip_gate_controller(void* arg);

void* left_right_path(void *arg)
{
	struct info *args = (struct info *)arg;
	int sid = args->pass_id;
	bool vip = args->vip_staus;
	int timer = args->timer;

	pthread_mutex_lock(&mut);
	time(&ending);
    double time_taken = double(ending- start);
	cout<<"Passenger "<<sid<<(vip?"(VIP)":"")<<" has started waiting to use VIP gate (left->right) at "<<round(time_taken)<<endl;
	pthread_mutex_unlock(&mut);

	sem_wait(&left_right_sem);
	left_right_passengers++;
	if(left_right_passengers == 1)
	{
		sem_wait(&gate_sync_sem);
	}
	sem_post(&left_right_sem);

	pthread_mutex_lock(&mut);
	time(&ending);
    time_taken = double(ending- start);
	cout<<"Passenger "<<sid<<(vip?"(VIP)":"")<<" has started to use VIP gate (left->right) at "<<round(time_taken)<<endl;
	pthread_mutex_unlock(&mut);

	timer+=z;
	sleep(z);

	pthread_mutex_lock(&mut);
	time(&ending);
    time_taken = double(ending- start);
	cout<<"Passenger "<<sid<<(vip?"(VIP)":"")<<" has finished using VIP gate (left->right) at "<<round(time_taken)<<endl;
	pthread_mutex_unlock(&mut);

	sem_wait(&left_right_sem);
	left_right_passengers--;
	if(left_right_passengers == 0)
	{
		sem_post(&gate_sync_sem);
	}
	sem_post(&left_right_sem);


	//start boarding thread
	struct info temp;
	temp.pass_id = sid;
	temp.thread_priority = 0;
	temp.vip_staus = vip;
	temp.timer = timer;

	pthread_t boarding_thread;
	pthread_create(&boarding_thread,NULL,&boarding_func,(void*)new struct info(temp));

	pthread_join(boarding_thread,NULL);

	return NULL;
}

void* right_left_path(void *arg)
{
	struct info *args = (struct info *)arg;
	int sid = args->pass_id;
	bool vip = args->vip_staus;
	int timer = args->timer;

	pthread_mutex_lock(&mut);
	time(&ending);
    double time_taken = double(ending- start);
	cout<<"Passenger "<<sid<<(vip?"(VIP)":"")<<" has started waiting to use VIP gate (right->left) at "<<round(time_taken)<<endl;
	pthread_mutex_unlock(&mut);

	sem_wait(&right_left_sem);
	right_left_passengers++;
	if(right_left_passengers == 1)
	{
		sem_wait(&gate_sync_sem);
	}
	sem_post(&right_left_sem);

	pthread_mutex_lock(&mut);
	time(&ending);
    time_taken = double(ending- start);
	cout<<"Passenger "<<sid<<(vip?"(VIP)":"")<<" has started to use VIP gate (right->left) at "<<round(time_taken)<<endl;
	pthread_mutex_unlock(&mut);

	timer+=z;
	sleep(z);

	pthread_mutex_lock(&mut);
	time(&ending);
    time_taken = double(ending- start);
	cout<<"Passenger "<<sid<<(vip?"(VIP)":"")<<" has finished using VIP gate (right->left) at "<<round(time_taken)<<endl;
	pthread_mutex_unlock(&mut);

	sem_wait(&right_left_sem);
	right_left_passengers--;
	if(right_left_passengers == 0)
	{
		sem_post(&gate_sync_sem);
	}
	sem_post(&right_left_sem);

	//wait 1 sec
	timer+=1;
	sleep(1);

	//go to the special kiosk for new boarding pass
	pthread_mutex_lock(&mut);
	time(&ending);
    time_taken = double(ending- start);
	cout<<"Passenger "<<sid<<(vip?"(VIP)":"")<<" has started waiting at special kiosk at "<<round(time_taken)<<endl;
	pthread_mutex_unlock(&mut);

	sem_wait(&special_kiosk_sem);

	pthread_mutex_lock(&mut);
	time(&ending);
    time_taken = double(ending- start);
	cout<<"Passenger "<<sid<<(vip?"(VIP)":"")<<" has started using the special kiosk at "<<round(time_taken)<<endl;
	pthread_mutex_unlock(&mut);

	timer+=w;
	sleep(w);

	pthread_mutex_lock(&mut);
	time(&ending);
    time_taken = double(ending- start);
	cout<<"Passenger "<<sid<<(vip?"(VIP)":"")<<" has got another boarding pass from special kiosk at "<<round(time_taken)<<endl;
	pthread_mutex_unlock(&mut);

	sem_post(&special_kiosk_sem);


	//wait 1 sec
	timer++;
	sleep(1);

	//Use the VIP-gate(left->right) now

	//push in priority queue
	struct info temp;
	temp.pass_id = sid;
	temp.thread_priority = 1;
	temp.vip_staus = vip;
	temp.timer = timer;

	pthread_mutex_lock(&prior_mutex);
	pair<int,info> pair_one = make_pair(1,temp);
	prior_queue.push(make_pair(1,temp));
	sem_post(&prior_empty);

	pthread_mutex_unlock(&prior_mutex);

	//start vip gate controller thread
	pthread_t vip_gate_scheduler_thread;
	pthread_create(&vip_gate_scheduler_thread,NULL,vip_gate_controller,NULL);
	pthread_join(vip_gate_scheduler_thread,NULL);


	return NULL;
	
}

void* vip_gate_controller(void* arg)
{
	
	//downs prior_empty
	sem_wait(&prior_empty);

	//pop the element
	pthread_mutex_lock(&prior_mutex);

	pair<int,info> node = prior_queue.top();
	prior_queue.pop();

	pthread_mutex_lock(&mut);
	//cout<<"element popped -> passenger->"<<node.second.pass_id<<" priority->"<<node.first<<endl;
	pthread_mutex_unlock(&mut);

	pthread_mutex_unlock(&prior_mutex);


	//schedule the left->right and right->left threads
	int priority = node.first;
	pthread_t scheduled_thread;

	if(priority == 1)
	{
		pthread_create(&scheduled_thread,NULL,left_right_path,(void*)new struct info(node.second));
	}

	else if(priority == 0)
	{
		pthread_create(&scheduled_thread,NULL,right_left_path,(void*)new struct info(node.second));
	}

	pthread_join(scheduled_thread,NULL);
	return NULL;
	
}


void* boarding_func(void *arg)
{
	struct info *args = (struct info *)arg;
	//assigned unique id
	ll sid = args->pass_id;
	//assign vip status
	bool vip = args->vip_staus;
	//assign timer
	int timer = args->timer;

	//wait 1 sec
	timer+=1;
	sleep(1);

	//boarding phase

	//randomly lose the boarding pass (33.33% chance to lose it)
	int rand_val = rand()%3;
	bool lost = false;
	if(rand_val == 2)
		lost = true;

	if(!lost) //board the plane
	{
		pthread_mutex_lock(&mut);
		time(&ending);
    	double time_taken = double(ending- start);
		cout<<"Passenger "<<sid<<(vip?"(VIP)":"")<<" has not lost his boarding pass"<<endl;
		cout<<"Passenger "<<sid<<(vip?"(VIP)":"")<<" is waiting to to board the plane at "<<round(time_taken)<<endl;
		pthread_mutex_unlock(&mut);

		//board the plane
		
		sem_wait(&boarding_gate);

		pthread_mutex_lock(&mut);
		time(&ending);
    	time_taken = double(ending- start);
		cout<<"Passenger "<<sid<<(vip?"(VIP)":"")<<" started to board the plane at "<<round(time_taken)<<endl;
		pthread_mutex_unlock(&mut);

		timer+=y;
		sleep(y);

		pthread_mutex_lock(&mut);
		time(&ending);
    	time_taken = double(ending- start);
		cout<<"Passenger "<<sid<<(vip?"(VIP)":"")<<" has boarded the plane at "<<round(time_taken)<<endl;
		pthread_mutex_unlock(&mut);

		sem_post(&boarding_gate);

	}

	else //go back through VIP line
	{
		pthread_mutex_lock(&mut);
		cout<<"Passenger "<<sid<<" has lost his boarding pass"<<endl;
		pthread_mutex_unlock(&mut);


		//push in the priority queue
		struct info temp;
		temp.pass_id = sid;
		temp.thread_priority = 0;
		temp.vip_staus = vip;
		temp.timer = timer;

		pthread_mutex_lock(&prior_mutex);

		//0->low priority
		//1->high priority

		prior_queue.push(make_pair(0,temp));
		sem_post(&prior_empty);

		pthread_mutex_unlock(&prior_mutex);

		//start vip gate controller thread
		pthread_t vip_gate_scheduler_thread;
		pthread_create(&vip_gate_scheduler_thread,NULL,vip_gate_controller,NULL);
		pthread_join(vip_gate_scheduler_thread, NULL);

	}


	return NULL;

}




void* passenger_func(void* arg)
{
	//sleep(1);

	
	struct info *args = (struct info *)arg;

	//assigned unique id
	ll sid = args->pass_id;
	//assign vip status
	bool vip = args->vip_staus;
	//assign timer
	int timer = args->timer;
	
	//print arrival
	pthread_mutex_lock(&mut);

	time(&ending);
    double time_taken = double(ending- start);
	//assigned unique id
	// sid = id;	
	// id++;
	cout<<"Passenger "<<sid<<(vip?"(VIP)":"")<<" arrived at airport at "<<time_taken<<endl;
	//cout<<"mid: "<<sid<<"="<<args->pass_id<<", priority: "<<sid<<"= "<<args->thread_priority<<endl;
	delete(args);

	pthread_mutex_unlock(&mut);
	
	//wait 1 sec
	// timer++;
	// sleep(1);


	//kiosk check-in
	sem_wait(&kiosk);
	
	pthread_mutex_lock(&mut);

	time(&ending);
    time_taken = double(ending- start);
	cout<<"Passenger "<<sid<<(vip?"(VIP)":"")<<" has started self check-in at kiosk "<<kiosk_no<<" at "<<time_taken<<endl;

	
	pthread_mutex_unlock(&mut);

	//increment kiosk no
	pthread_mutex_lock(&kiosk_update);
	kiosk_no++;
	if(kiosk_no > M)
		kiosk_no = 1;
	pthread_mutex_unlock(&kiosk_update);


	timer+=w;
	sleep(w);

	//recieve boarding pass
	pthread_mutex_lock(&mut);

	time(&ending);
    time_taken = double(ending- start);
	cout<<"Passenger "<<sid<<(vip?"(VIP)":"")<<" got his boarding pass at "<<time_taken<<endl;

	pthread_mutex_unlock(&mut);

	sem_post(&kiosk);

	//wait 1 sec
	timer+=1;
	sleep(1);

	//security_part

	//randomly choose a belt-no
	int belt_no = rand()%N;

	if(!vip) //go through security belt
	{
		//started waiting at security checkpoint
		pthread_mutex_lock(&mut);

		time(&ending);
    	double time_taken = double(ending- start);
		cout<<"Passenger "<<sid<<(vip?"(VIP)":"")<<" has started waiting for security check in belt "<<belt_no+1<<" at "<<time_taken<<endl;

		pthread_mutex_unlock(&mut);

		sem_wait(&security_sem[belt_no]);
		
		pthread_mutex_lock(&mut);

		time(&ending);
    	time_taken = double(ending- start);
		cout<<"Passenger "<<sid<<(vip?"(VIP)":"")<<" started security check in belt "<<belt_no+1<<" at "<<time_taken<<endl;

		pthread_mutex_unlock(&mut);

		timer+=x;
		sleep(x);


		//cleared security check
		pthread_mutex_lock(&mut);

		time(&ending);
		time_taken = double(ending- start);
		cout<<"Passenger "<<sid<<(vip?"(VIP)":"")<<" has crossed security check at "<<round(time_taken)<<endl;

		pthread_mutex_unlock(&mut);

		sem_post(&security_sem[belt_no]);


		struct info temp;
		temp.pass_id = sid;
		temp.thread_priority = 1;
		temp.vip_staus = vip;
		temp.timer = timer;

		pthread_t boarding_thread;
		pthread_create(&boarding_thread,NULL,&boarding_func,(void*)new struct info(temp));

		pthread_join(boarding_thread,NULL);
	}

	else //skip security and go through VIP channel
	{
		//push in priority queue
		struct info temp;
		temp.pass_id = sid;
		temp.thread_priority = 1;
		temp.vip_staus = vip;
		temp.timer = timer;

		pthread_mutex_lock(&prior_mutex);
		pair<int,info> pair_one = make_pair(1,temp);
		prior_queue.push(make_pair(1,temp));
		sem_post(&prior_empty);

		pthread_mutex_unlock(&prior_mutex);

		//start vip gate controller thread
		pthread_t vip_gate_scheduler_thread;
		pthread_create(&vip_gate_scheduler_thread,NULL,vip_gate_controller,NULL);
		pthread_join(vip_gate_scheduler_thread,NULL);
	}

	return NULL;

}

int main(void)
{	
	
	//read from files
	string filename("input.txt");
	ifstream input_file(filename);
    if (!input_file.is_open()) {
        cerr << "Could not open the file - '"
             << filename << "'" << endl;
        return EXIT_FAILURE;
    }

	
	input_file>>M;
	input_file>>N;
	input_file>>P;
	input_file>>w;
	input_file>>x;
	input_file>>y;
	input_file>>z;

	cout<<M<<N<<P<<w<<x<<y<<z<<endl;

    input_file.close();

	//init_semaphore();
	//printing semaphore
	pthread_mutex_init(&mut,NULL);
	//kiosk check-in semaphores
	pthread_mutex_init(&kiosk_update,NULL);
	sem_init(&kiosk,0,M);
	//security check sempahores
	for(int i=0;i<N;i++)
	{
		sem_t temp;
		sem_init(&temp,0,P);
		security_sem.push_back(temp);
	}
	//boarding semaphore
	sem_init(&boarding_gate,0,1);
	//priority_queue accessing semaphore
	pthread_mutex_init(&prior_mutex, NULL);
	sem_init(&prior_empty,0,0);
	//VIP gate semaphores
	sem_init(&left_right_sem,0,1);
	sem_init(&right_left_sem,0,1);
	sem_init(&gate_sync_sem,0,1);
	//special kiosk semaphore
	sem_init(&special_kiosk_sem,0,1);
	

	

	/*poisson starts*/
	std::random_device rd; // uniformly-distributed integer random number generator
 	std::mt19937 rng (rd ()); // mt19937: Pseudo-random number generation

	srand(time(0));

	double no_of_passengers;
	cout<<"Input total number of passengers: "<<endl;
	cin>>no_of_passengers;

	double averageArrival;
	//double averageArrival = 15;
	cout<<"Input Average Arrival Time: "<<endl;
	cin>>averageArrival;

	//poisson mean -> lambda
	double lamda = 1 / averageArrival;
	std::exponential_distribution<double> exp (lamda);

	double sumArrivalTimes=0;
	double newArrivalTime;
	int interval;

	vector<pthread_t> threads;
	int mid = 1;


	time(&start);
	int timer = 0;
	for (int i = 0; i < no_of_passengers; i++)
	{
		newArrivalTime=  exp.operator() (rng);// generates the next random number in the distribution
		interval = round(newArrivalTime);
		//cout<<"interval: "<<interval<<endl;
		timer+=interval;
		sleep(interval);

		//make thread
		pthread_t passenger_thread;

		//randomly assign vip status (33.33% chance)
		int rand_val = rand()%3;
		bool vip = false;
		if(rand_val == 2)
			vip = true;

		struct info *args = new struct info();
		args->pass_id = mid;
		args->vip_staus = vip;
		args->thread_priority = 0;
		args->timer = timer;
		
		++mid;
		//dispatch thread
		//pthread_setschedprio(passenger_thread,1);
		pthread_create(&passenger_thread,NULL,&passenger_func,(void*)args);
		
		//save all the threads
		threads.push_back(passenger_thread);
		
	}


	//join all the threads with main thread
	for(pthread_t pt:threads)
	{
		pthread_join(pt,NULL);
	}

	//while(1);

	return 0;
}


