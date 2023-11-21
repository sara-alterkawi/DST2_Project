#include "kernel_functions.h"

// *****************************************************************************
//                 LAB THREE
// *****************************************************************************

exception wait(uint nTicks){ //Block the calling task until the given number of ticks has expired
  isr_off(); //Disable interrupt
  //nTCnt is timer counter"sleep timer"
  ReadyList->pHead->nTCnt = (uint)(nTicks+Ticks); //Current ticker + sleeptime = finished sleeping time
  PreviousTask = ReadyList->pHead->pTask; //Update PreviousTask
  sortedInsertion(&TimerList,extract(&ReadyList)); //Place running task in the TimerList
  NextTask = ReadyList->pHead->pTask; //Update NextTask
  SwitchContext(); //Switch context
  if(ReadyList->pHead->pTask->Deadline < Ticks) //IF deadline is reached
    return DEADLINE_REACHED;
  else
    return OK;
}

void set_ticks(uint nTicks){ //Set the tick counter to the given value.
  Ticks = nTicks; //Set the tick counter.
}

uint ticks(void){ //Return the current value of the tick counter
  return Ticks; //Return the tick counter
}

uint deadline(void){ //Return the deadline of the specified task
  return ReadyList->pHead->pTask->Deadline; //Return the deadline of the current task
}

void set_deadline(uint deadline){//Set the deadline for the calling task
  isr_off(); //Disable interrupt
  ReadyList->pHead->pTask->Deadline = deadline; //Set the deadline field in the calling TCB
  PreviousTask = ReadyList->pHead->pTask; //Update PreviousTask
  sortedInsertion(&ReadyList,extract(&ReadyList)); //Reschedule ReadyList
  NextTask = ReadyList->pHead->pTask; //Update NextTask
  SwitchContext(); //Switch context
}

void TimerInt(){ //
  Ticks++; //Increment tick counter
  if(TimerList->pHead != NULL){ //Check the TimerList for tasks
  listobj *temp = TimerList->pHead;
  
  while(temp != NULL){
    if(temp->nTCnt < ticks() || temp->pTask->Deadline < ticks()){ //Sleeptime not over
      listobj *get = temp; //Get the node
      temp = temp->pNext; //Go to next
      PreviousTask = ReadyList->pHead->pTask;
      get = getNode(&TimerList,get); //Retrieve node
      sortedInsertion(&ReadyList,get); //Insert node
      NextTask = ReadyList->pHead->pTask; //Update NextTask
    }
    else 
      temp = temp->pNext; //Go to next
  }    
 }
  
 if(WaitingList->pHead != NULL){
  listobj *tempwait = WaitingList->pHead; //Proceed to Waitinglist
  
  while(tempwait != NULL){
    if(tempwait->pTask->Deadline < ticks()){
      listobj *get = tempwait;
      tempwait = tempwait->pNext;
      PreviousTask = ReadyList->pHead->pTask;
      get = getNode(&WaitingList,get);
      sortedInsertion(&ReadyList,get);
      NextTask = ReadyList->pHead->pTask;
      
    }
    else
      tempwait = tempwait->pNext;
  }   
 }
}

