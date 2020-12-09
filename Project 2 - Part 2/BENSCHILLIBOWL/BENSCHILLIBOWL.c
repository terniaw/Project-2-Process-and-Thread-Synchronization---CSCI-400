#include "BENSCHILLIBOWL.h"

#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/shm.h>

bool IsEmpty(BENSCHILLIBOWL* bcb);
bool IsFull(BENSCHILLIBOWL* bcb);
void AddOrderToBack(Order **orders, Order *order);
// void AddOrderToBack(BENSCHILLIBOWL *bcb, Order *order);

MenuItem BENSCHILLIBOWLMenu[] = { 
    "BensChilli", 
    "BensHalfSmoke", 
    "BensHotDog", 
    "BensChilliCheeseFries", 
    "BensShake",
    "BensHotCakes",
    "BensCake",
    "BensHamburger",
    "BensVeggieBurger",
    "BensOnionRings",
};
int BENSCHILLIBOWLMenuLength = 10;
int ShmID;

/* Select a random item from the Menu and return it */
MenuItem PickRandomMenuItem() {
    int randNum = (rand() % BENSCHILLIBOWLMenuLength);
    return BENSCHILLIBOWLMenu[randNum];
}

/* Allocate memory for the Restaurant, then create the mutex and condition variables needed to instantiate the Restaurant */

BENSCHILLIBOWL* OpenRestaurant(int max_size, int expected_num_orders) {
  printf("Restaurant is open!\n");
  
  BENSCHILLIBOWL* ShmPTR;
  BENSCHILLIBOWL* bowl;
  
  ShmID = shmget(IPC_PRIVATE, sizeof(BENSCHILLIBOWL), IPC_CREAT | 0666);
  if (ShmID<0){
    printf("*** shmget error ***\n");
    exit(1);
  }
  bowl = (BENSCHILLIBOWL *) shmat(ShmID, NULL, 0);
  
  bowl->orders = NULL;
  bowl->current_size = 0;
  bowl->max_size = max_size;
  bowl->next_order_number = 0;
  bowl->orders_handled = 0;
  bowl->expected_num_orders = expected_num_orders;
  bowl->mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
  bowl->can_add_orders = (pthread_cond_t) PTHREAD_COND_INITIALIZER;
  bowl->can_get_orders = (pthread_cond_t) PTHREAD_COND_INITIALIZER;  
  
  return bowl;
}


/* check that the number of orders received is equal to the number handled (ie.fullfilled). Remember to deallocate your resources */

void CloseRestaurant(BENSCHILLIBOWL* bcb) {
  if (bcb->expected_num_orders == bcb->orders_handled){
    pthread_mutex_destroy(&(bcb->mutex));
    pthread_cond_destroy(&(bcb->can_add_orders));
    pthread_cond_destroy(&(bcb->can_get_orders));
    shmdt((void *) bcb);
    shmctl(ShmID, IPC_RMID, NULL);
    printf("Restaurant is closed!\n");
  } else {
    printf("Cannot close restaurant until all orders are done \n");
  }
}

/* add an order to the back of queue */
int AddOrder(BENSCHILLIBOWL* bcb, Order* order) {
  pthread_mutex_lock(&(bcb->mutex));
  while (IsFull(bcb)){
    pthread_cond_wait(&(bcb->can_add_orders), &(bcb->mutex));
  }
  if (bcb->orders == NULL){
    bcb->orders = order;
    order->next = NULL;
  }else{
    AddOrderToBack(&bcb->orders, order);
  }
  order->order_number = bcb->next_order_number;
  
  bcb->current_size+=1;
  bcb->next_order_number+=1;
  
  pthread_cond_broadcast(&(bcb->can_get_orders));
  pthread_mutex_unlock(&(bcb->mutex));
  return order->order_number;
}

/* remove an order from the queue */
Order *GetOrder(BENSCHILLIBOWL* bcb) {
  pthread_mutex_lock(&(bcb->mutex));
  
  while (IsEmpty(bcb)){
    if (bcb->orders_handled ==  bcb->expected_num_orders){
      pthread_mutex_unlock(&(bcb->mutex));
      return NULL;
    }
    pthread_cond_wait(&(bcb->can_get_orders), &(bcb->mutex));
  }
  
  Order* order = bcb->orders;
  bcb->orders = order->next;
  bcb->current_size-=1;
  bcb->orders_handled+=1;
  
  pthread_cond_broadcast(&(bcb->can_add_orders));
  pthread_mutex_unlock(&(bcb->mutex));
  
  return order;
}

// Optional helper functions (you can implement if you think they would be useful)
bool IsEmpty(BENSCHILLIBOWL* bcb) {
  return (bcb->orders == NULL);
}

bool IsFull(BENSCHILLIBOWL* bcb) {
  int max_size = bcb->max_size;
  Order* current_order = bcb->orders;
  int counter = 0;
  while (current_order != NULL){
    counter+=1;
    current_order = current_order->next; 
  }
  return (max_size == counter);
}

/* this methods adds order to rear of queue */
void AddOrderToBack(Order **orders, Order *order) {
  order->next = NULL;
  Order* current_order = *orders;
  while (current_order->next != NULL){
    current_order = current_order->next;
  }
  current_order->next = order;
}

