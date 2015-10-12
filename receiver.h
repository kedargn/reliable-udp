#ifndef RECEIVER_H
#define RECEIVER_H

#include<stdio.h>
#include"rudp.h"

/** strcuture to maintain the receiver's state
*/
typedef struct receiver_state_vars{
 int last_byte_received;
 int last_byte_acked;
 int last_byte_read;
 int next_byte_expected;
}receiver_state;

/** intializes the server state
*/
void initialize_receiver(receiver_state *receiver){
  receiver->next_byte_expected = 0;
  receiver->last_byte_received = 0;
  receiver->last_byte_acked = receiver->last_byte_read = 0;
}

/** helper function to update the receiver state
*/
receiver_state update_receiver_state(receiver_state receiver, struct rudp_header header_info ){
  if(receiver.next_byte_expected%SEQ_WRAP_UP == header_info.seq_no%SEQ_WRAP_UP){
	 receiver.next_byte_expected += header_info.data_length+1;
	 receiver.last_byte_read += header_info.data_length;
   receiver.last_byte_received += header_info.data_length;
  }
  return receiver;
}
#endif
