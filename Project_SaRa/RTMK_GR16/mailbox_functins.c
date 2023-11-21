#include "kernel_functions.h"

// *****************************************************************************
//                 LAB TWO
// *****************************************************************************

msg *createMsg(void *pData){//Creates a message with given data 
  msg *temp;
  temp = (msg *) malloc(sizeof(msg));
  temp->pData = pData;
  temp->pBlock = NULL;
  temp->pNext = NULL;
  temp->pPrevious = NULL;
  temp->Status = 0;
  return temp;
}

exception insertMail(mailbox **mBox,msg **message){//FIFO Inserts a message into the mailbox
  if((*mBox) == NULL || message == NULL)
    return FAIL;
  
  if((*mBox)->nMaxMessages == (*mBox)->nBlockedMsg || (*mBox)->nMaxMessages == (*mBox)->nMessages)
    return NO_SPACE; //Returns no space exception
  
  (*message)->pNext = NULL;
  (*message)->pPrevious = NULL; 
  
  if((*mBox)->pHead == NULL && (*mBox)->pHead == (*mBox)->pTail){   
    (*mBox)->pHead = (*message);
    (*mBox)->pTail = (*message);
    return OK;
    
  }else if((*mBox)->pHead != NULL && (*mBox)->pHead == (*mBox)->pTail){ //Contains one mail
    (*mBox)->pHead->pNext = (*message);
    (*message)->pPrevious = (*mBox)->pHead;
    (*mBox)->pTail = (*message);
    return OK;
    
  }else{
    (*mBox)->pTail->pNext = (*message);
    (*message)->pPrevious = (*mBox)->pTail;
    (*mBox)->pTail = (*message);
    (*mBox)->pTail->pNext = NULL;
    return OK;
    
  }
}

msg* getMsg(mailbox **mBox){//Extracts the top message from the mailbox
  if((*mBox) == NULL || ((*mBox)->nMessages + (*mBox)->nBlockedMsg) == 0)
    return FAIL;
  
  else{
    if((*mBox)->pHead->pNext == NULL){//WGetting the last message
      msg *message = (*mBox)->pHead;
      (*mBox)->pHead = NULL;
      (*mBox)->pTail = NULL;
      (*mBox)->pHead->pNext = NULL;
      (*mBox)->pTail->pNext = NULL;
      (*mBox)->pHead->pPrevious = NULL;
      (*mBox)->pTail->pPrevious = NULL;
      message->pNext = NULL;
      message->pPrevious = NULL;
      return message;
      
    }else{
      msg *message = (*mBox)->pHead;
      (*mBox)->pHead = (*mBox)->pHead->pNext;
      (*mBox)->pHead->pPrevious = NULL;
      message->pNext = NULL;
      message->pPrevious = NULL; 
      return message;
    }
  }
}

// *****************************************************************************

mailbox *create_mailbox(uint nMessages,uint nDataSize){//Create a mailbox and return it
  mailbox *temp;
  temp = (mailbox *) malloc(sizeof(mailbox)); //Allocate memory for the mailbox
  temp->nDataSize = nDataSize; //Initialize mailbox structure
  temp->nMaxMessages = nMessages;
  temp->nBlockedMsg = 0;
  temp->nMessages = 0;  
  temp->pHead = NULL;
  temp->pTail = NULL;
  return temp; //Return *mailbox
}

exception remove_mailbox(mailbox* mBox){//Removes the mailbox
  if(mBox->nBlockedMsg > 0 || mBox->nMessages > 0)
    return NOT_EMPTY;//Cant remove if it contains data
  
  else{ //Mailbox is empty
    free(mBox);
    return OK;
  }
}

exception send_wait(mailbox* mBox,void* pData){
  isr_off(); //Disable interrupt
  if(mBox == NULL || pData == NULL || (mBox->nBlockedMsg > 0 && mBox->nMessages > 0))
    return FAIL;
  
  if(mBox->nBlockedMsg == mBox->nMaxMessages || mBox->nMessages > 0)
    return FAIL; //if its full or if it contains non-waiting messages
  
  if(mBox->pHead->Status == RECEIVER){//IF receiving task is waiting
    msg *receivingMsg = getMsg(&mBox); //Get top message
    listobj *receivingNode = getNode(&WaitingList,receivingMsg->pBlock);//Get the blocked Node
    memcpy(receivingMsg->pData,pData,mBox->nDataSize); //Copy sender’s data to the data area of the receivers Message
    receivingNode->pMessage = receivingMsg; //Remove receiving task’s Message struct from the mailbox
    PreviousTask = ReadyList->pHead->pTask;//Update PreviousTask
    sortedInsertion(&ReadyList,receivingNode); //Insert receiving task in ReadyList
    NextTask = ReadyList->pHead->pTask; //Update NextTask
  }else{
    msg *message = createMsg(pData);
    message->Status = SENDER;
    message->pBlock = ReadyList->pHead;
    if(insertMail(&mBox,&message) == OK) //Add Message to the mailbox
      mBox->nBlockedMsg++;
    
    else
      return FAIL;
    
    PreviousTask = ReadyList->pHead->pTask; //Update PreviousTask
    listobj *moveNode = extract(&ReadyList);
    sortedInsertion(&WaitingList,moveNode); //Move sending task from ReadyList to WaitingList
    NextTask = ReadyList->pHead->pTask; //Update NextTask
  }
  SwitchContext(); //Switch context
  if(ReadyList->pHead->pTask->Deadline < Ticks){//IF deadline is reached.
    isr_off(); //Disable interrupt
    free(getMsg(&mBox));//Remove send Message
    isr_on(); //Enable interrupt
    return DEADLINE_REACHED;
  }
  else return OK;
}

exception receive_wait(mailbox *mBox,void* pData){
  isr_off(); //Disable interrupt
  if(mBox == NULL)
    return FAIL;
  if(mBox->pHead->Status == SENDER){//IF send Message is waiting
    msg *sendingMsg = getMsg(&mBox);
    memcpy(pData,sendingMsg->pData,mBox->nDataSize); //Copy sender’s data to receiving task’s data area.
    if(sendingMsg->pBlock != NULL){
      mBox->nBlockedMsg--;
      listobj *Node = getNode(&WaitingList,sendingMsg->pBlock); //Remove sending task’s Message struct from the mailbox.
      PreviousTask = ReadyList->pHead->pTask; //Update PreviousTask
      sortedInsertion(&ReadyList,Node); //Move sending task to ReadyList
      NextTask = ReadyList->pHead->pTask; //Update NextTask
    }else{
      mBox->nMessages--;
      free(sendingMsg); //Free senders data area
    }
  }else{
    msg *receivingMsg = (NULL);
    receivingMsg->Status = RECEIVER; //Allocate a Message structure
    receivingMsg->pBlock = ReadyList->pHead;
    if(insertMail(&mBox,&receivingMsg) == OK)
      mBox->nBlockedMsg++; //Add Message to the mailbox
    else 
      return FAIL;
    PreviousTask = ReadyList->pHead->pTask; //Update PreviousTask
    listobj *moveNode = extract(&ReadyList); //Move receiving task from ReadyList to WaitingList
    sortedInsertion(&WaitingList,moveNode);
    NextTask = ReadyList->pHead->pTask; //Update NextTask
  }
  SwitchContext(); //Switch context
  if(ReadyList->pHead->pTask->Deadline < Ticks){//IF deadline is reached
    isr_off(); //Disable interrupt
    free(getMsg(&mBox));//Remove receive Message
    mBox->nBlockedMsg--;
    isr_on(); //Enable interrupt
    return DEADLINE_REACHED;
  }
  else return OK;  
}

exception send_no_wait(mailbox* mBox,void* pData){
  isr_off(); //Disable interrupt
  if(mBox == NULL || (mBox->nBlockedMsg > 0))
    return FAIL;
  
  if(mBox->pHead->Status == RECEIVER){ //IF receiving task is waiting
    msg *receivingMsg = getMsg(&mBox);
    listobj *receivingNode = getNode(&WaitingList,receivingMsg->pBlock);
    mBox->nBlockedMsg--;
    memcpy(receivingMsg->pData,pData,mBox->nDataSize); //Copy data to receiving tasks’ data area.
    receivingNode->pMessage = receivingMsg;
    PreviousTask = ReadyList->pHead->pTask; //Update PreviousTask
    sortedInsertion(&ReadyList,receivingNode); //Move receiving task to Readylist
    NextTask = ReadyList->pHead->pTask; //Update NextTask
    SwitchContext(); //Switch context
  }else{
    char *data = (char *) malloc(sizeof(char)); //Allocate a Message structure
    memcpy(data,pData,mBox->nDataSize); //Copy Data to the Message
    msg *sendingMsg = createMsg(data);
    sendingMsg->Status = SENDER;
    if(mBox->nMessages == mBox->nMaxMessages) //IF mailbox is full
      //free(getMsg(&mBox)); //Remove the oldest Message
      //**************************************
    { 
      msg *oldestMsg = getMsg(&mBox); //Get the oldest message
      free(oldestMsg->pData); //Free the message data
      free(oldestMsg); //Free the message struct
      mBox->nMessages--; //Decrement the nMessages counter
    }
    //****************************************
    if(insertMail(&mBox,&sendingMsg) == FAIL)
      return FAIL;
    
    else 
      mBox->nMessages++; //Add Message to the mailbox
  }
  return OK;
}

exception receive_no_wait(mailbox *mBox,void* pData){
  isr_off(); //Disable interrupt
  if(mBox == NULL || (mBox->nMessages == 0 && mBox->nBlockedMsg == 0))
    return FAIL;
  if(mBox->pHead->Status == SENDER){ //IF send Message is waiting
    msg *sendingMsg = getMsg(&mBox);
    memcpy(pData,sendingMsg->pData,mBox->nDataSize); //Copy sender’s data to receiving task’s data area.
    if(sendingMsg->pBlock != NULL){
      mBox->nBlockedMsg--; //Remove sending task’s Message struct from the mailbox
      PreviousTask = ReadyList->pHead->pTask; //Update PreviousTask
      listobj *sendingNode = getNode(&WaitingList,sendingMsg->pBlock); //Move sending task to ReadyList
      sortedInsertion(&ReadyList,sendingNode); //
      NextTask = ReadyList->pHead->pTask; //Update NextTask
      SwitchContext(); //SwitchContext
    }else{
      mBox->nMessages--;
      free(sendingMsg); //Free sender's data area
    }
  }
  else return FAIL;
  return OK;
}
