#ifndef SENDER_H
#define SENDER_H

#include<stdio.h>
#include"rudp.h"
#include<sys/socket.h>
#include<arpa/inet.h>

typedef struct sender_state_variables{
 int last_byte_acked;
 int next_byte_to_be_acked;
 int last_byte_sent;
 int last_file_byte_acked;
 int next_byte;
 int eof;
}sender_state;

typedef struct sender_packets{
 int total;
 int retransmitted;
 int slow_start;
 int cong_avoid;
 int once;
}packets;


//extern sender_state initialize_state(sender_state, char*, char*);
struct rudp_header create_header_info(sender_state, int, int);

sender_state update_sender(sender_state sender, int length){
 sender.last_byte_sent = length;
 return sender;
}

packets initialize_packets_counter(packets p){
	p.total = p.retransmitted = p.slow_start = p.cong_avoid = p.once = 0;
	return p;
}

void initialize_state(sender_state *sender,char*  destn_ip, int destn_port){
 sender->last_byte_acked = 0;
 sender->next_byte_to_be_acked=PAYLOAD+1;
 sender->last_byte_sent = 0;
 sender->last_file_byte_acked = 0;
 sender->eof=0;
}

void prepare_header(unsigned char* header, sender_state state, int data_length, int retransmit){
   struct rudp_header header_info;
   header_info  = create_header_info(state, data_length, retransmit);
   makeHeader(header, header_info);
}

struct rudp_header create_header_info(sender_state state, int data_length, int retransmit){
 struct rudp_header header_info;
 header_info.ack = 2;
 header_info.ack_no = 0;
 header_info.seq_no = (retransmit == 0 ?(state.next_byte%SEQ_WRAP_UP) : (state.last_byte_acked%SEQ_WRAP_UP)); //(state.next_byte == 0 ? 0 : state.next_byte);
 header_info.data_length = data_length;
 header_info.eof = state.eof; 
 return header_info;
}
#endif
