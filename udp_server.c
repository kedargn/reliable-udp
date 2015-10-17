#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<math.h>
#include<pthread.h>
#include<sys/time.h>
#include<math.h>
#include"rudp.h"
#include"sender.h"
#define ALPHA 0.125
#define BETA 0.25

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

int sock, port = 65000, client_window=50*PAYLOAD, cong_window=1*PAYLOAD;
int congestion_state = SLOW_START, ssthresh = 64000;
int is_thread = -1, retransmit =-1;
int client_addr_length, drtt = 0, ertt = 0, file_length = 0;
char request[MSS];// = "GET /persistent.txt HTTP/1.1\nHost: sadsa.dsadsa.com\nConnection: alive\n\n";
char* response;
char file_name[50], headers[50];
char* file_contents;
int total_packets = 0, retransmitted_packets = 0, slow_start_packets = 0, cong_avoid_packets = 0, sent_file_bytes = 0; 
sender_state sender;
packets packets_count;
struct timeval rtt;
float drop_probability = 0.0;

int skipped = 0, not_skipped=0;

/**
 * checks and assigns the command line arguments
 */
void read_args(int argc, char *argv[]){
  if(argc != 4){
    printf("Invalid number of arguements\n. Please enter port no., client window and drop probability\n");
    exit(1);
  }
  port = atoi(argv[1]);
  client_window = (atoi(argv[2]))*PAYLOAD;
  drop_probability = atof(argv[3]);
  printf("Server port %d\nClient Window %d\nDrop Probability %f\n", port, client_window, drop_probability);
}


int main(int argc, char *argv[])
{
 pthread_t thread1; 
 int received_bytes, file_found;
 server_addr.sin_family = AF_INET;
 server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
 server_addr.sin_port = htons(port);
 client_addr_length = sizeof(client_addr); 
 rtt.tv_sec = 1;
 rtt.tv_usec = 0;
 read_args(argc, argv);
 create_socket();
 bind_socket();
 packets_count = initialize_packets_counter(packets_count);
 printf("SOCKET RUNNING AT PORT %d\n", port);
 for(;;){
  printf("waiting to receive data from client \n");
  received_bytes = recvfrom(sock, request, MSS,0, (struct sockaddr*)&client_addr, &client_addr_length);
  if(received_bytes>0){
    get_file_name();
    reply();
  }
 }
 close(sock);
 exit(1);
}

/**
 * Calls the functions responsible for transfering file to client
 */ 
void reply()
{
 struct rudp_header header_info;
 header_info = getHeaderInfo(request);
 if(header_info.ack==1) {
  mark_ack(header_info);
  return;
 }
 read_file();
 send_response(header_info);
}

/**
 * Prints the result after the file has been transmitted
 */
void print_result(){
  printf("Total Packets Transmitted %lu\n", packets_count.total);
  printf("Total Packets Once %d\n", packets_count.once);
  printf("Total Packets Retransmitted %d\n", packets_count.retransmitted);
  printf("Total Packets in Slow Start %lu and percentage %f\n", packets_count.slow_start, ((double)packets_count.slow_start/(double)packets_count.total)*100);
  printf("Total Packets Congestion Avoidance %lu and percentage %f\n\n", packets_count.cong_avoid, ((double)packets_count.cong_avoid/(double)packets_count.total)*100);
  printf("Total Packets Fast Recovery %lu and percentage %f\n\n", packets_count.fast_recovery, ((double)packets_count.fast_recovery/(double)packets_count.total)*100);
  printf("Number of Dropped Packets %d\n Number of not Dropped Packets %d\n", skipped, not_skipped);
}

/**
 * Increments packets count whenever packets are transmitted
 * Keeps track of packets transmitted in total, retarnsmitted, slow start stage and congestion avoidance stage
 * @param retransmit:  1 - > Segment is retransmitted
 *                     0 -> Segment transmitted for first time
 */
void increment_packets_count(int retransmit){
  packets_count.total++;
  if(retransmit == 1){
    packets_count.retransmitted++;
  } else{
    packets_count.once++;
  }
  (congestion_state == SLOW_START) ? packets_count.slow_start++ : ((congestion_state == FAST_RECOVERY) ? packets_count.fast_recovery++ : packets_count.cong_avoid++);
}

/**
 * Trasnmits the segment start from index to PAYLOAD bytes from file
 * @param index: starting byte of file
 * @param retransmit:  1 - > Segment is retransmitted
 *                     0 -> Segment transmitted for first time
 */

void transmit(int index, int retransmit){
  if(nextBool(drop_probability)==-1){ //nextBool(0.5)==0
    not_skipped++;
    if(((file_length/PAYLOAD)==(index/PAYLOAD))){
      sender.eof = 1;
    }else{
      sender.eof = 0;
    }
    prepare_header(headers, sender, PAYLOAD, retransmit);
    response = (char*)calloc(MSS, sizeof(char));
    memcpy(response, headers, HEADER_LENGTH);
    memcpy(&response[HEADER_LENGTH], &file_contents[index], PAYLOAD);
    sendto(sock, response, MSS, 0, (struct sockaddr*)&client_addr, sizeof(client_addr)); 
 } else {
  skipped++;
 }
 increment_packets_count(retransmit);
}

/**
 * Increments congestion window based on the current congestion stage
 */ 
void increment_cong_window(){
  float increment;
  if(congestion_state == SLOW_START){
    printf("IN SLOW START\n");
    cong_window += PAYLOAD;
    if(cong_window >= ssthresh){
      printf("SWITCHING TO CONGESTION AVOIDANCE\n");
      congestion_state = CONGESTION_AVOIDANCE;
    }
  } else if(congestion_state == CONGESTION_AVOIDANCE){
      printf("IN CONGESTION AVOIDANCE\n");
      increment = ((float)PAYLOAD/cong_window)*PAYLOAD;
      cong_window += ceil(increment);
  }
}

/**
 * Routineto shift to slow start in case of timeout event
 */ 
void go_to_slow_start(){
  sender.dup_acks = 0;
  sender.dup_ack_byte = -1;
  ssthresh = (cong_window)/2;
  cong_window = 1*PAYLOAD;
  printf("SWITCHING to SLOW START\n");
}

/**
 * Routine to mark the segment as acknowledged
 * @param: struct rudp_header: Structure contains the acknowledgement header info from client
 */ 
void mark_ack(struct rudp_header header_info){
  if(sender.next_byte_to_be_acked%SEQ_WRAP_UP == header_info.ack_no){
    increment_cong_window();
    sender.last_byte_acked = sender.next_byte_to_be_acked;
    sender.last_file_byte_acked += PAYLOAD;
    sender.next_byte_to_be_acked += PAYLOAD+1;
  }
  else {
    transmit(sender.last_file_byte_acked, 1);
  }
}


void print_header(struct rudp_header header_info){
 //printf("ACK RECEIVED-->ack %d, ack_no %d,seq_no %d, data_length %d, eof %d\n",header_info.ack,header_info.ack_no,header_info.seq_no,header_info.data_length, header_info.eof);
}


/**calculates and updates the RTT
 * @param sent and received: used to derive sample RTT
 */ 
void calculate_rtt(struct timeval *sent, struct timeval *received){
  long mseconds = 0;
  mseconds = (received->tv_sec - sent->tv_sec)*1000000;
  mseconds += (received->tv_usec - sent->tv_usec);
  ertt = (1-0.125)*ertt+(0.125)*(mseconds);
  drtt = (0.75)*drtt + (0.25)*abs(mseconds-ertt);
  mseconds = ertt + 4*drtt;
  rtt.tv_sec=(mseconds/10000000);
  rtt.tv_usec = (mseconds%1000000==0) ? 5000 : (mseconds%1000000);
}

/**
 * Routine waits for acks for timeout interval
 * In case of timeout it restransmits the segment and calls the routine to shift to slow start stage
 */
int wait_for_an_ack(){
  unsigned char ack_content[MSS];
  int size;
  struct rudp_header header_info;
  struct timeval sent, received;
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&rtt, sizeof(struct timeval));
  gettimeofday(&sent, NULL);
  size = recvfrom(sock, ack_content, MSS,0, (struct sockaddr*)&client_addr, &client_addr_length);
  gettimeofday(&received, NULL);
  calculate_rtt(&sent, &received);
  if(size<=0){
    transmit(sender.last_file_byte_acked,1);
    go_to_slow_start();
    sent_file_bytes = sender.last_file_byte_acked + PAYLOAD;
    sender.next_byte = (sent_file_bytes/PAYLOAD)+sent_file_bytes;
    sender.last_byte_sent = sent_file_bytes;
    return -1;
  }
  else{
    header_info = getHeaderInfo(ack_content);
    retransmit = -1;              
    if(header_info.ack==ACK){
     if(sender.next_byte_to_be_acked%SEQ_WRAP_UP == header_info.ack_no) {
      if(congestion_state == FAST_RECOVERY){
        sender.dup_acks = 0;
        sender.dup_ack_byte = -1;
        congestion_state = CONGESTION_AVOIDANCE;
        cong_window = ssthresh;
      }
      mark_ack(header_info);
      return 0;
    }else {
      if(sender.dup_ack_byte !=-1){
        sender.dup_ack_byte = header_info.ack_no;
        sender.dup_acks++;
      }else if(sender.dup_ack_byte == header_info.ack_no) {
        ((sender.dup_acks >= 3) ? sender.dup_acks : sender.dup_acks++);
      }else{
       sender.dup_ack_byte = header_info.ack_no; 
       sender.dup_acks = 1;
      }
      if(sender.dup_acks == 3){
        if(congestion_state == FAST_RECOVERY){
          cong_window += 1*PAYLOAD;
        } else {
        congestion_state = FAST_RECOVERY;
        ssthresh = (cong_window)/2;
        cong_window = ssthresh + (3*PAYLOAD);
        }
        transmit(sender.last_file_byte_acked,1);   
      }
      return -1;
    }
   }
  }
}

/** returns minimum of receiver and congestion window
 */
int min(){
  return cong_window > client_window ? client_window/PAYLOAD : cong_window/PAYLOAD;
}

/**
 * interates through file and sends response based on minimum of receiver and congestion window
 * slides window as the acknoledgemnts are received
 */
void send_response(struct rudp_header header_info)
{
  int temp;
 int client_addr_len, i = 1, timeout;
 initialize_state(&sender, NULL, 0);
 client_addr_len = sizeof(client_addr);
 response = (char*)calloc(MSS, sizeof(char));
 file_length = strlen(file_contents);

  while(sent_file_bytes <= file_length){
   for(i=0;i<min();i++){
    if(sent_file_bytes <= file_length){
      transmit(sent_file_bytes, 0);
      sender.next_byte += PAYLOAD+1;
      sent_file_bytes += PAYLOAD;
      sender.last_byte_sent += PAYLOAD;
    }
   } 
   temp = min();
   for(i=0;i<temp && sender.last_file_byte_acked < sent_file_bytes;i++){
      timeout = wait_for_an_ack();
   }

   while(sender.last_file_byte_acked < sent_file_bytes){
    timeout = wait_for_an_ack();
   } 
  }
  free(response);
  print_result();
  exit(1);
 }

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
 * Helper function to get filename requested by client
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
