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
char file_name[50] = "cn2.txt", destn_file_name[50]="destn.txt";
int sock, total_bytes_received=0, port;
char* file_contents;
unsigned char request[100], response[MSS+1], server_ip[30]="127.0.0.1";


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
 prepare_header(header, sender, strlen(file_name), 0);
 header[HEADER_LENGTH] = '\0';
 memcpy(request, header, HEADER_LENGTH);
 memcpy(&request[HEADER_LENGTH], file_name, strlen(file_name));
 sendto(sock, request, MSS, 0, (struct sockaddr*)&server_addr, sizeof(server_addr)); 
 sender = update_sender(sender, strlen(header)+strlen(file_name));
}

void receive_data()
{
 //file_name = (char*)malloc()
 FILE *fp;
 printf("File delete result is %d\n",remove(destn_file_name));
 fp = fopen(destn_file_name, "a");
 struct rudp_header ack_header;
 struct rudp_header header_info;
 int simulate=0;
 int length;
 int client_addr_length = sizeof(client_addr);
 int bytes_per_request=0;
 for(;;)
 {
  bytes_per_request = recvfrom(sock, response, MSS,0, (struct sockaddr*)&client_addr, &client_addr_length);
  response[MSS+1]='\0';
  header_info = getHeaderInfo(response);
  print_header(header_info);
  printf("NEXT EXPECTED BYTE %d and ack no%d\n", receiver.next_byte_expected, receiver.next_byte_expected%SEQ_WRAP_UP);
  if((receiver.next_byte_expected%SEQ_WRAP_UP) == (header_info.seq_no%SEQ_WRAP_UP)){
    // receiver = update_receiver_state(receiver, header_info);
    if(header_info.ack != 1){
      //printf("X is %d\n", simulate);
      if(1==0){ //nextBool(0.4)==0
        sleep(0.5);
        printf("****ACK WILL NOT BE SENT****\n");
      }else{
        receiver = update_receiver_state(receiver, header_info);
        send_ack();
        fputs(&response[10], fp);
        fflush(fp);
        // /printf("%s\n", &response[10]);
        if(header_info.eof != 0){
          fclose(fp);
          printf("File Transfer Completed: %s\n", file_name);
          exit(1);
        }
      }
    }
  }
    //printf("%s\n", &response[9]);
   else if(receiver.next_byte_expected > header_info.seq_no){
    printf("ALREADY RECEIVED SEGMENT\n");
    print_header(header_info);
    //send_ack();
  } else{
    printf("OUT OF ORDER SEGMENT. SENDING DUPLICATE ACK\n");
    print_header(header_info);
    send_ack();
   }
 }
 fclose(fp);
}

void print_header(struct rudp_header header_info){
 printf("ack %d, ack_no %d,seq_no %d, data_length %d, eof %d\n",header_info.ack,header_info.ack_no,header_info.seq_no,header_info.data_length, header_info.eof);
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
 ack_header.ack_no = receiver.next_byte_expected%SEQ_WRAP_UP;
 ack_header.seq_no = sender.last_byte_sent+1;
 sender.last_byte_sent++;
 ack_header.data_length = 1;
 ack_header.eof = 10;
 makeHeader(header, ack_header);
 printf("***sending following ack***\n");
 print_header(ack_header);
 sendto(sock, header, HEADER_LENGTH, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
}
