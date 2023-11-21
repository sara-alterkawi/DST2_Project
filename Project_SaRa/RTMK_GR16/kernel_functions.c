#include "kernel_functions.h"
#include <stdlib.h>

// *****************************************************************************
//                 LAB ONE
// *****************************************************************************

int Ticks; /* global sysTick counter */
int KernelMode; /* can be equal to either INIT or RUNNING (constants defined 3 * in “kernel_functions.h”)*/
TCB *PreviousTask, *NextTask; /* Pointers to previous and next running tasks */
list *ReadyList, *WaitingList, *TimerList;

list *emptyList(){ //Initializes an empty list and returns it
  list *temp;
  temp = (list *) malloc(sizeof(list));
  temp->pHead = NULL;
  temp->pHead->pNext = NULL;
  temp->pHead->pPrevious = NULL;
  temp->pTail = NULL;
  temp->pTail->pNext = NULL;
  temp->pTail->pPrevious = NULL;
  return temp;
}

exception initList(){ //Initializes the three main lists
  ReadyList = emptyList();
  WaitingList = emptyList();
  TimerList = emptyList();
  if(ReadyList == NULL || WaitingList == NULL || TimerList == NULL)
    return FAIL;
  else
    return OK;
}

listobj *createNode(TCB *task){ //Creates a new list node containing a pointer to a TCB
  listobj *temp;
  temp = (listobj *) malloc(sizeof(listobj));
  temp->pTask = task;
  temp->pMessage = NULL;
  temp->pNext = NULL;
  temp->pPrevious = NULL;
  temp->nTCnt = 0;  
  return temp;
}

int getSize(list *List){ //Returns the size of a list
  int size = 0;
  listobj *head = List->pHead;
  while(head != NULL){
    head = head->pNext;
    size++;
  }
  return size;
}

listobj *extract(list **List){ //Removes returns the head node of a list and removes it from the list
  listobj *temp = (*List)->pHead;
  
  (*List)->pHead = (*List)->pHead->pNext;
  (*List)->pHead->pPrevious = NULL;
  temp->pNext = NULL;
  temp->pPrevious = NULL;
  
  if((*List)->pHead == (*List)->pTail){//Set next and prev to null
    (*List)->pHead->pNext = NULL;
    (*List)->pTail->pNext = NULL;
    (*List)->pTail->pPrevious = NULL;
  }
  return temp;
}

exception nullLista(list **List){//Refere if last node is removed
  (*List)->pHead->pNext = NULL;
  (*List)->pHead->pPrevious = NULL;
  (*List)->pTail->pNext = NULL;
  (*List)->pTail->pPrevious = NULL;
  return OK;
}

exception sortedInsertion(list **List, listobj *Node){//Inserts a new node with a task in a sorted position in the list
  if((*List) == NULL || Node == NULL)
    return FAIL;
  
  if(getSize(*List) == 0){
    (*List)->pHead = Node;
    (*List)->pTail = Node;
    return OK;
  }
  else if(getSize(*List) == 1){
    if(Node->pTask->Deadline < (*List)->pHead->pTask->Deadline){//If list contains one entry, Node will be our next head
      Node->pNext = (*List)->pTail;
      (*List)->pTail->pPrevious = Node;
      (*List)->pHead = Node;
      (*List)->pHead->pPrevious = NULL;
      return OK;
    }
    else{//Head lower deadline
      Node->pPrevious = (*List)->pHead;
      (*List)->pHead->pNext = Node;
      (*List)->pTail = Node;
      return OK;
    }
  }
  else{
    listobj *traNode = (*List)->pHead;
    while(traNode != NULL && traNode->pTask->Deadline < Node->pTask->Deadline){
      traNode = traNode->pNext;
    }
    if(traNode == (*List)->pHead && Node->pTask->Deadline <= traNode->pTask->Deadline){//If we traveled to head
      Node->pNext = (*List)->pHead;
      (*List)->pHead->pPrevious = Node;
      (*List)->pHead = Node;
      Node->pPrevious = NULL;
      return OK;
    }
    else if(traNode == NULL){//When reach the tail
      Node->pPrevious = (*List)->pTail;
      (*List)->pTail->pNext = Node;
      (*List)->pTail = Node;
      Node->pNext = NULL;
      return OK;
    }
    else{
      traNode->pPrevious->pNext = Node;
      Node->pPrevious = traNode->pPrevious;   
      Node->pNext = traNode;
      traNode->pPrevious = Node;
      return OK;
    }
  }
}

listobj* getNode(list **List,listobj *Node){//Removes and returns the node given by a pointer
  if(getSize(*List) == 0 || (*List) == NULL || Node == NULL)
    return NULL; //The list is empty/Node is NULL
  else{
    if(getSize(*List) == 1){//It is only one node in the list
      listobj *temp = (*List)->pHead;
      (*List)->pHead = NULL;
      (*List)->pTail = NULL;
      nullLista(&(*List));
      temp->pNext = NULL;
      temp->pPrevious = NULL;
      return temp;
    }
    else if(getSize(*List) == 2){//only two nodes left
      if((*List)->pHead == Node){
        listobj *temp = (*List)->pHead;
        (*List)->pHead = (*List)->pTail;
        nullLista(&(*List));
        temp->pNext = NULL;
        temp->pPrevious = NULL;
        return temp;
      }
      else{
        listobj *temp = (*List)->pTail;
        (*List)->pTail = (*List)->pHead;
        nullLista(&(*List));
        temp->pNext = NULL;
        temp->pPrevious = NULL;
        return temp;
      }
    }
    else{//More than 2 Nodes
      listobj *traNode = (*List)->pHead;
      while(traNode != Node){
        traNode = traNode->pNext;
      }
      if(traNode == (*List)->pHead){
        (*List)->pHead = (*List)->pHead->pNext;
        (*List)->pHead->pPrevious = NULL;
        traNode->pNext = NULL;
        traNode->pPrevious = NULL;
        return traNode;
      }
      else if(traNode == (*List)->pTail){
        (*List)->pTail = (*List)->pTail->pPrevious;
        (*List)->pTail->pNext = NULL;
        traNode->pNext = NULL;
        traNode->pPrevious = NULL;
        return traNode;
      }
      else{
        traNode->pPrevious->pNext = traNode->pNext;
        traNode->pNext->pPrevious = traNode->pPrevious;
        traNode->pNext = NULL;
        traNode->pPrevious = NULL;
        return traNode;
      }
    }
  }
}

// *****************************************************************************

exception init_kernel(){//Initialize Kernel 
  Ticks = 0; //Set tick counter to zero
  if(initList() == FAIL)
    return FAIL;
  //Create necessary data structure
  if(create_task(idle, UINT_MAX) == FAIL)
    return FAIL;
  //if this fails return fail
  else 
    KernelMode = INIT; //Set the kernel in INIT mode
  return OK; //Return status
}

exception create_task (void (*taskBody)(), unsigned int deadline){ //the create task function
  TCB *new_tcb;
  new_tcb = (TCB *) calloc (1, sizeof(TCB));
  if(new_tcb == NULL){ /* you must check if calloc was successful or not! */
    return FAIL;
  }
  new_tcb->PC = taskBody;
  new_tcb->SPSR = 0x21000000;
  new_tcb->Deadline = deadline;
  
  new_tcb->StackSeg [STACK_SIZE - 2] = 0x21000000;
  new_tcb->StackSeg [STACK_SIZE - 3] = (unsigned int) taskBody;
  new_tcb->SP = &(new_tcb->StackSeg [STACK_SIZE - 9]);
  
  // after the mandatory initialization you can implement the rest of the suggested pseudocode
  
  if(KernelMode == INIT){ //Insert new task in ReadyList
    if(sortedInsertion(&ReadyList,createNode(new_tcb)) == FAIL)
      return FAIL;
    else
      return OK;
  }else{
    isr_off(); //Disable interrupts
    PreviousTask = ReadyList->pHead->pTask; //Update PreviousTask
    if(sortedInsertion(&ReadyList,createNode(new_tcb)) == FAIL)
      return FAIL;
    else{
      NextTask = ReadyList->pHead->pTask; //Update NextTask
      SwitchContext(); //Switch context 
      return OK;
    }
  }
}

void run(){//Function to run
  Ticks = 0; //Initialize interrupt timer
  KernelMode = RUNNING; //Set KernelMode = RUNNING
  NextTask = ReadyList->pHead->pTask; //Set NextTask to equal TCB of the task to be loaded
  LoadContext_In_Run(); //Load context
}

void terminate(){//function to terminate tasks
  isr_off(); //Disable interrupts
  listobj *newNode = extract(&ReadyList); //Remove running task from ReadyList (get new head)
  NextTask = ReadyList->pHead->pTask; //Set NextTask to equal TCB of the next task (set next task to the new heads TCB)
  switch_to_stack_of_next_task(); //Switch to process stack of task to be loaded
  
  free(newNode->pTask); //Remove data structures of task being terminated (free the retrieved Node and task "old head")
  free(newNode);
  LoadContext_In_Terminate(); //Load context
}

void idle(){//simple idle task
  while(1);
} 

