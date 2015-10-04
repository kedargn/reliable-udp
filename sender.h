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
 struct sockaddr_in destn_addr;
 int next_byte;
}sender_state;

//extern sender_state initialize_state(sender_state, char*, char*);
struct rudp_header create_header_info(sender_state, int, int);

sender_state update_sender(sender_state sender, int length){
 sender.last_byte_sent = length;
 return sender;
}

void initialize_state(sender_state *sender,char*  destn_ip, int destn_port){
 sender->last_byte_acked = 0;
 sender->next_byte_to_be_acked=PAYLOAD+1;
 sender->last_byte_sent = 0;
 sender->last_byte_sent = 0;
 //sender->destn_addr.sin_family = PF_INET;
 //sender->destn_addr.sin_port = htons(destn_port);
 //sender->destn_addr.sin_addr.s_addr = inet_addr(destn_ip);
 //return sender;
}

void prepare_header(unsigned char* header, sender_state state, int data_length, int is_ack){
 if(is_ack!=1){
   struct rudp_header header_info;
   header_info  = create_header_info(state, data_length, is_ack);
   makeHeader(header, header_info);
 }else{
  
 }
}

struct rudp_header create_header_info(sender_state state, int data_length, int is_ack){
 struct rudp_header header_info;
 header_info.ack = 2;
 header_info.ack_no = 0; //TODO:
 header_info.seq_no = state.next_byte;//(state.next_byte == 0 ? 0 : state.next_byte);
 header_info.data_length = data_length;
 header_info.adv_window = 1234; //TODO
 return header_info;
}
/**
NOT USED
**/
int send_data_to(int sock, sender_state state, char* header, char* payload){
 char mss[MSS];
 strcat(mss, header);
 strcat(mss, payload);
 sendto(sock, mss, strlen(mss), 0, (struct sockaddr*)&(state.destn_addr), sizeof(state.destn_addr));
 state.last_byte_sent += strlen(mss);
 state.next_byte += (strlen(mss)+1);
}
#endif
