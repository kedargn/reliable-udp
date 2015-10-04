#include<stdlib.h>
#include<stdio.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<string.h>
#include"sender.h"
#include"receiver.h"

void send_data();
void receive_data();
void create_socket();
void check_result();
void send_ack();
void print_header(struct rudp_header);
sender_state sender;
receiver_state receiver;

struct sockaddr_in server_addr, client_addr;
char file_name[50] = "hello.txt";
int sock, total_bytes_received=0, port;
unsigned char request[100], response[MSS+1], server_ip[30]="127.0.0.1";


//extern sender_state  initialize_state(sender_state, char*, char* );

int main(int argc, char* argv[])
{
 initialize_state(&sender, server_ip, port);
 initialize_receiver(&receiver);
 printf("Server IP Address %s\nPort %d\nFilename %s\n",server_ip, port, file_name);
 server_addr.sin_family = PF_INET;
 server_addr.sin_addr.s_addr = inet_addr(server_ip);
 server_addr.sin_port = htons(65000);
 create_socket();
 send_data();
 receive_data();
 close(sock);
}


void send_data(){
 unsigned char header[HEADER_LENGTH];
 /*struct rudp_header header_info;
 header_info.ack_no = 12345;
 header_info.ack=2;
 header_info.seq_no = 7891;
 header_info.adv_window = 7070;
 header_info.data_length = 8729;
 */
 //printf("aw %d\nseq no %d\nack %d\nack_no %d\n data len %d\n", header_info.adv_window, header_info.seq_no, header_info.ack, header_info.ack_no, header_info.data_length);

 prepare_header(header, sender, strlen(file_name), 0);
 //send_data_to(sock, sender, header, file_name);
 header[HEADER_LENGTH] = '\0';
 strcat(request, header);
 printf("header length %d\n",(int)strlen(header));
 strcat(request, file_name);
 printf("request length %d\n", (int)strlen(request));
 sendto(sock, request, strlen(request), 0, (struct sockaddr*)&server_addr, sizeof(server_addr)); 
 sender = update_sender(sender, strlen(header)+strlen(file_name));
}

void receive_data()
{
 struct rudp_header ack_header;
 struct rudp_header header_info;
 int length;
 int client_addr_length = sizeof(client_addr);
 int bytes_per_request=0;
 for(;;)
 {
  bytes_per_request = recvfrom(sock, response, MSS,0, (struct sockaddr*)&client_addr, &client_addr_length);
  response[MSS+1]='\0';
  header_info = getHeaderInfo(response);
  print_header(header_info);
  receiver = update_receiver_state(receiver, header_info);
  if(header_info.ack != 1){
   send_ack();
  }
  //printf("%s\n", &response[9]);
 }
}

void print_header(struct rudp_header header_info){
 printf("ack %d, ack_no %d,seq_no %d, data_length %d, adv_window %d\n",header_info.ack,header_info.ack_no,header_info.seq_no,header_info.data_length, header_info.adv_window);
}
void create_socket()
{
 sock = socket(PF_INET, SOCK_DGRAM, 0);
 check_result("socket creation", sock);
}


void check_result(char* msg, int result)
{
if(result<0)
 {
  printf("%s failed with value %d\n",msg, result);
  perror(msg);
  exit(1);
 }
 else
 {
  printf("%s successful with value %d\n", msg, result);
 }
}

void send_ack(){
 struct rudp_header ack_header;
 unsigned char header[HEADER_LENGTH];

 ack_header.ack=1;
 ack_header.ack_no = receiver.last_byte_received+1;
 ack_header.seq_no = sender.last_byte_sent+1;
 sender.last_byte_sent++;
 ack_header.data_length = 1;
 ack_header.adv_window=1234;
 makeHeader(header, ack_header);
 printf("***sending following ack***\n");
 print_header(ack_header);
 sendto(sock, header, strlen(header), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
}
