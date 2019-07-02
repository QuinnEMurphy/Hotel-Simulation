#include <iostream>
#include <cstdlib>
#include <ctime>
#include <pthread.h>  
#include <semaphore.h>
#include <cstdlib>
#include <string>
#include <windows.h>
#include <random>


using namespace std;
#define GUESTSMAX 10
#define ROOMS 5

//for print out of guests activities in the end
int poolguests = 0;
int restaurantguests = 0;
int fitnessguests = 0;
int businessguests = 0;

default_random_engine g;
uniform_int_distribution<int> distribution(0,9999); //random numbers generate 0-9999 
/*The same random numbers appear in the same order everytime the code is compiled and ran.
I could not find another way to generate them. Change the "9999" to a different number to 
get different random results.*/


int guestroomrecieved = 0; 		//room number that guest recieves after guest has gotten one
int guestroomcheckedout = 0;	//room number guest is checking out of

int guestcheckin = 0; //guest that is checking in
int guestcheckout = 0; //guest that is checking out

bool roomsavailabe[ROOMS] = {false,false,false,false,false}; //to check if anyrooms are available


sem_t roomAssigned;           	//to assign a room one at a time
sem_t CheckInCounter;           //to see if the check-in counter is available
sem_t CheckOutCounter;          //to see if the check-out counter is available
sem_t checkingreet;				//so guest is greeted after walking to the check-in counter
sem_t checkoutgreet;          	//so guest is greeted after walking to the check-out counter
sem_t availableRoom;            //to check if a room is available to check-in and to ensure that not more than 5 rooms are assigned since there a max of 5 rooms
sem_t roomcheckout;				//so room is checked-out in order
sem_t roomcheck;				//so a room is not being changed to false at the same time it is being checked
sem_t print; 					//so only one line is printed at a time;

void *Guest(void *guest){
	
	int gNumber = *(int*)guest;	//guest number
	
	while(true){
		if(sem_wait(&availableRoom)==0){
			while(true){
				if(sem_wait(&print)==0){
					printf("Guest %d gets in line at the check-in counter.\n",gNumber);
					sem_post(&print);
					break;
				}
			}
			break;
		}
	}
	
	while(true){
		if(sem_wait(&CheckInCounter)==0){
			guestcheckin = gNumber;
			sem_post(&checkingreet);
			break;
		}
	}
	while(true){
		if(sem_wait(&roomAssigned)==0){
			break;
		}
	}
	
	int act = distribution(g) % 4;
	string activity;
	int sleeptime = (distribution(g) % 3) + 1;
	
	switch(act){
		case 0:
			activity = "Swimming Pool";
			poolguests++;
			break;
		case 1:
			activity = "Restaurant";
			restaurantguests++;
			break;
		case 2:
			activity = "Fitness Center";
			fitnessguests++;
			break;
		case 3:
			activity = "Business Center";
			businessguests++;
			break;
	}
	
	while(true){
		if(sem_wait(&print)==0){
			printf("Guest %d entered the hotel %s.\n",gNumber,activity.c_str());
			sem_post(&print);
			break;
		}
	}
	while(true){
		if(sem_wait(&print)==0){
			printf("Guest %d is now sleeping for %d seconds.\n",gNumber,sleeptime);
			sem_post(&print);
			break;
		}
	}
	Sleep(sleeptime*1000);
	while(true){
		if(sem_wait(&print)==0){
			printf("Guest %d done sleeping.\n", gNumber);
			sem_post(&print);
			break;
		}
	}
	
	while(true){
		if(sem_wait(&CheckOutCounter)==0){
			while(true){
				if(sem_wait(&print)==0){
					printf("Guest %d approaches the check-out counter.\n",gNumber);
					sem_post(&print);
					break;
				}
			}
			guestcheckout = gNumber;
			sem_post(&checkoutgreet);
			break;
		}
	}

	while(true){
		if(sem_wait(&roomcheckout)==0){			
			while(true){
				if(sem_wait(&print)==0){
					printf("Guest %d is now checked out!\n\n",gNumber);
					sem_post(&print);
					break;
				}
			}		
			sem_post(&availableRoom);
			break;
		}
	}
}
	
		


void *Checkin(void *checkarg){
	int guestscheckedin = 0;
	
	while(true){
		if(sem_wait(&checkingreet)==0){
			while(true){
				if(sem_wait(&print)==0){
					printf("Check-in counter greets guest %d.\n", guestcheckin);
					sem_post(&print);
					break;
				}
			}
			while(true){
				if(sem_wait(&roomcheck)==0){	//waits to see if anyone is changing a room to occupied(true) or unoccupied(false)
					for(int i = 0; i < ROOMS; i++){
						if(!roomsavailabe[i]){
							guestroomrecieved = i;
							roomsavailabe[i] = true;
							break;
						}
					}
					sem_post(&roomcheck);
					break;
				}
			}
			while(true){
				if(sem_wait(&print)==0){
					printf("Guest %d is checked into room %d.\n", guestcheckin, (guestroomrecieved + 1));
					sem_post(&print);
					break;
				}
			}
			
			sem_post(&roomAssigned);
			
			
			guestscheckedin++;
			Sleep(200);
			sem_post(&CheckInCounter);
			if(guestscheckedin >= GUESTSMAX){
				break;
			}
			
		}
	}
}



void *Checkout(void *checkarg){
	int guestscheckedout = 0;
	while(true){
		if(sem_wait(&checkoutgreet)==0){ //waits for guest to approach check-out desk
			srand(time(NULL));
			int price = (distribution(g) % 501 + 200);	//calculating price (200-700)
			while(true){
				if(sem_wait(&print)==0){
					printf("Check-out counter greets guest %d.\n", guestcheckout);
					sem_post(&print);
					break;
				}
			}
			while(true){
				if(sem_wait(&roomcheck)==0){		//waits to see if anyone is changing a room to occupied(true) or unoccupied(false)
					roomsavailabe[guestroomcheckedout] = false;
					sem_post(&roomcheck);
					break;
				}
			}
			while(true){
				if(sem_wait(&print)==0){
					printf("Guest %d pays $%d.\n", guestcheckout, price);
					sem_post(&print);
					break;
				}
			}
			sem_post(&CheckOutCounter);
			sem_post(&roomcheckout);
			guestscheckedout++;
			if(guestscheckedout >= GUESTSMAX){
				break;
			}
		}
	}
}


int main(){
	int guests[GUESTSMAX]; 				//10 guests
	for(int i = 0;i < GUESTSMAX; i++){	//initiate guest numbers
		guests[i] = i +1;
		
	}
	
	distribution(g); //first number generated is always zero
	
	pthread_t checkinthread;			// Check-in thread
	pthread_t checkoutthread;			// Check-out thread
	pthread_t guestthreads[GUESTSMAX];	// 10 guest threads
	
	
	
	
	for(int i = 0;i < GUESTSMAX; i++){        //create guest threads
		pthread_create(&guestthreads[i], NULL, Guest, (void*)&guests[i]);
		
	}
	pthread_create(&checkinthread, NULL, Checkin, NULL);
	pthread_create(&checkoutthread, NULL, Checkout, NULL);	

	sem_init(&availableRoom,0,ROOMS); 	//inital available rooms is 5
	sem_init(&roomAssigned,0,0);		//assigning room assignment to 0 so that the assignment waits for a guest to approach
	sem_init(&print,0,1); 				//only one line can be printed at a time;
	
	sem_init(&CheckInCounter,0,1); 		//only one person at the check-in desk
	sem_init(&CheckOutCounter,0,1);		//only one person at the check-out desk
	
	sem_init(&checkingreet,0,0);		//signaled after guest checks if check-in desk is available
	sem_init(&checkoutgreet,0,0);		//signaled after guest checks if check-out desk is available
	
	sem_init(&roomcheck,0,1);			
	sem_init(&roomcheckout,0,0);
				
 
	
	
	pthread_join(checkinthread, NULL);		//join check-in thread
	pthread_join(checkoutthread, NULL);		//join check-out thread
	for(int i = 0;i < GUESTSMAX; i++){		//join guest threads
		pthread_join(guestthreads[i], NULL);
		
	}
	
	
	
	
	cout << "Total Guests: " << GUESTSMAX <<endl;
	cout << "Pool: " << poolguests <<endl;
	cout << "Restaurant: " << restaurantguests <<endl;
	cout << "Fitness Center: " << fitnessguests <<endl;
	cout << "Business Center: " << businessguests <<endl;
	return 0;
}














































