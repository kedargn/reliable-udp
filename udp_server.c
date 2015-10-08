#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<math.h>
#include<pthread.h>
#include<sys/time.h>
#include"rudp.h"
#include"sender.h"

void create_socket();
void bind_socket();
void check_result(char*, int);
int get_file_name();
void reply();
void read_file();
int get_file_name();
void send_response(struct rudp_header);
void prep_headers(struct rudp_header);
void mark_ack(struct rudp_header);
struct sockaddr_in server_addr;
struct sockaddr_in client_addr;

int sock, port = 65000, client_window;
int is_thread = -1, retransmit =-1;
int client_addr_length;
char request[MSS];// = "GET /persistent.txt HTTP/1.1\nHost: sadsa.dsadsa.com\nConnection: alive\n\n";
char* response;
char file_name[50], headers[50];
char* file_contents;
sender_state sender;



void *poll_acks(){
  printf("****NEW THREAD****\n");
  int received_bytes;
  unsigned char ack[MSS];
  struct rudp_header header_info;
  for(;;){
   received_bytes = recvfrom(sock,ack, MSS,0, (struct sockaddr*)&client_addr, &client_addr_length);
   header_info = getHeaderInfo(ack);
   if(header_info.ack==ACK){
    //mark_ack(header_info);
   }
  else{
    printf("ERROR, NOT AN ACK RECEIVED\n");
  }
  //print_header(header_info);
 } 
}


int main()
{
 pthread_t thread1; 
 int received_bytes, file_found;
 server_addr.sin_family = AF_INET;
 server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
 server_addr.sin_port = htons(port);
 client_addr_length = sizeof(client_addr); 
 //printf("enter the PORT number\n");
 //scanf("%d",&port);
 create_socket();
 bind_socket();
 printf("SOCKET RUNNING AT PORT %d\n", port);
 for(;;){
  printf("waiting to receive data from client \n");
  received_bytes = recvfrom(sock, request, MSS,0, (struct sockaddr*)&client_addr, &client_addr_length);
  if(received_bytes>0){//printf("the request is %s\n", request);
    get_file_name();
    //printf("filename is %s\n",file_name);
    if(is_thread==-1){
     //received_bytes = pthread_create(&thread1, NULL, poll_acks, NULL);
     is_thread = 0;
   }
    reply();
  }
 }
 close(sock);
 exit(1);
}

 
void reply()
{
 struct rudp_header header_info;
 header_info = getHeaderInfo(request);
 client_window = header_info.adv_window;
 if(header_info.ack==1) {
  mark_ack(header_info);
  return;
 }
 printf("CLIENT HEADER-->aw %d\nseq no %d\nack %d\nack_no %d\n data len %d\n", header_info.adv_window, header_info.seq_no, header_info.ack, header_info.ack_no, header_info.data_length); 
 read_file();
 send_response(header_info);
}

void transmit(int index, int retransmit){
  if(nextBool(0.5)==0){
    prepare_header(headers, sender, PAYLOAD, retransmit);
    response = (char*)calloc(MSS, sizeof(char));
    strcat(response, headers);
    strncat(response, &file_contents[index], PAYLOAD);
    sendto(sock, response, MSS, 0, (struct sockaddr*)&client_addr, sizeof(client_addr)); 
 } else {
  printf("LET US SKIP\n");
 }
}

void mark_ack(struct rudp_header header_info){
  if(sender.next_byte_to_be_acked == header_info.ack_no){
    sender.last_byte_acked = sender.next_byte_to_be_acked;
    sender.last_file_byte_acked += PAYLOAD;
    sender.next_byte_to_be_acked += PAYLOAD+1;
    printf("ACK RECEIVED-->ack %d, ack_no %d,seq_no %d, data_length %d, adv_window %d\n",header_info.ack,header_info.ack_no,header_info.seq_no,header_info.data_length, header_info.adv_window);
    printf("NEXT EXPECTED ACK %d\n", sender.next_byte_to_be_acked);
  }
  else {
    printf("OUT OF ORDER ACK RECEIVED-->ack %d, ack_no %d,seq_no %d, data_length %d, adv_window %d\n",header_info.ack,header_info.ack_no,header_info.seq_no,header_info.data_length, header_info.adv_window);
    printf("NEXT EXPECTED ACK %d\n", sender.next_byte_to_be_acked);
    transmit(sender.last_file_byte_acked, 1);
    //wait_for_an_ack();
  }
}
void print_header(struct rudp_header header_info){
 //printf("ACK RECEIVED-->ack %d, ack_no %d,seq_no %d, data_length %d, adv_window %d\n",header_info.ack,header_info.ack_no,header_info.seq_no,header_info.data_length, header_info.adv_window);
}

int wait_for_an_ack(){
  unsigned char ack_content[MSS];
  int size;
  struct rudp_header header_info;
  struct timeval timeout;
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;
  printf("ENTERED wait_for_ack()\n");

  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(struct timeval));
  size = recvfrom(sock, ack_content, MSS,0, (struct sockaddr*)&client_addr, &client_addr_length);
  if(size<=0){
    printf("ACK SOCKET TIMEDOUT\n");
    transmit(sender.last_file_byte_acked,1);
    //wait_for_an_ack();
    return -1;
  }
  else{
    header_info = getHeaderInfo(ack_content);
    client_window = header_info.adv_window;
    retransmit = -1;                //not retransmitting this segment
    if(header_info.ack==ACK){
     if(sender.next_byte_to_be_acked == header_info.ack_no) {
      mark_ack(header_info);
      return 0;
    }else {
      printf("OUT OF ORDER ACK RECEIVED-->ack %d, ack_no %d,seq_no %d, data_length %d, adv_window %d\n",header_info.ack,header_info.ack_no,header_info.seq_no,header_info.data_length, header_info.adv_window);
      printf("NEXT EXPECTED ACK %d\n", sender.next_byte_to_be_acked);
      printf("Retransmitting byte %d\n", sender.last_file_byte_acked);
      transmit(sender.last_file_byte_acked,1);
      //wait_for_an_ack();
      return -1;
    }
   }
  }
}


void send_response(struct rudp_header header_info)
{
 int client_addr_len,file_length, sent_file_bytes = 0, i = 1, timeout;
 initialize_state(&sender, NULL, 0);
 client_addr_len = sizeof(client_addr);
 response = (char*)calloc(MSS, sizeof(char));
 file_length = strlen(file_contents);

 if(file_length<=PAYLOAD){			//TODO: Might cause silly window syndrome
  prepare_header(headers, sender, strlen(file_contents),0);
  strcat(response, headers);
  strcat(response, file_contents);
  sendto(sock, response, MSS, 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
  sender.last_byte_sent += MSS;
 }
 else {
  while(sent_file_bytes <= file_length){
   for(i=0;i<client_window;i++){
    if(sent_file_bytes <= file_length){
      printf("sending file byte %d\n", sent_file_bytes);
      transmit(sent_file_bytes, 0);
      sender.next_byte += PAYLOAD+1;
      sent_file_bytes += PAYLOAD;
      sender.last_byte_sent += PAYLOAD;
    }
   } 
   
   for(i=0;i<client_window && sender.last_file_byte_acked <= sent_file_bytes;i++){
      timeout = wait_for_an_ack();
   }

   while(sender.last_file_byte_acked <= sent_file_bytes){
    printf("window is %d\n",client_window);
    printf("sender.last_file_byte_acked --> %d sent_file_bytes --> %d\n", sender.last_file_byte_acked, sent_file_bytes);
    timeout = wait_for_an_ack();
   } 
  }
  free(response);
  exit(1);
  }
 }
 //free(response);



void prep_headers(struct rudp_header header_info)
{
 makeHeader(headers, header_info); //TODO:change header values
}

/**
 * Helper function to read the requested file.
 * The file contents will be copied to file_contents variable. 
 * Based on the size of file, the file_contents variable size will be dynamically increased so that it never runs out of memory
 **/
void read_file()
{
 FILE *fp;
 char ch;
 int i=0, file_contents_size;
 file_contents_size = 500;
 file_contents = (char*)calloc(file_contents_size, sizeof(char));
 fp = fopen(file_name, "r");
 if(fp == NULL){
  printf("FILE NOT FOUND\n");
  return;
 }
 while((ch=getc(fp))!=EOF)
 {
  file_contents[i++] = ch;
  if(i == (file_contents_size-5)){
    file_contents_size += file_contents_size;
    file_contents = realloc(file_contents, file_contents_size);
   }
 }
 file_contents[i] = '\0';
 //return 0;
}

/**
 * Helper function to get filename requested in HTTP request
 * if HTTP request is bad then is_request_valid will equated to -1 value
 * else file_name variable will be equated with requested file name
 **/
int get_file_name()
{
  strcpy(file_name,&request[10]);
  printf("filename is %s\n", file_name);
}

/**
 * creates socket
 **/
void create_socket()
{
 sock = socket(PF_INET, SOCK_DGRAM, 0);
 check_result("socket creation", sock);
}

/**
 * Binds socket.
 * If bind is unsuccesful then program will exit
 **/
void bind_socket()
{
 int bind_result;
 bind_result = bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
 check_result("Socket binding", bind_result);
}

/**
 * helper function to check results of socket creation and bind
 **/
void check_result(char* msg, int result)
{
if(result<0)
 {
  printf("%s failed with value %d\n",msg, result);
  perror(msg);
  exit(0);
 }
 else
 {
  printf("%s successful with value %d\n", msg, result);
 }
}
