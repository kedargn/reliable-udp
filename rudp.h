#ifndef RUDP_H
#define RUDP_H

#include<stdio.h>
#include<math.h>
#define HEADER_LENGTH 10
#define PAYLOAD 1463
#define MSS PAYLOAD+HEADER_LENGTH
#define ACK 1
#define SLOW_START 5
#define CONGESTION_AVOIDANCE 6
#define SEQ_WRAP_UP 60025
struct rudp_header{
 int ack;
 int ack_no;
 int seq_no;
 int eof;
 int data_length;
};

void insertBytes(int header_value,unsigned char *header, int first_byte, int second_byte){
 int a=0,b=0,c=0;
 a = (header_value & 65280)>>8;
 b = (header_value & 255);
 //a = a == 0 ? -1:a;
 //b = b == 0 ? -1:b;
 //printf("headervalue %d the value is %d and %d\n",header_value,a,b);
 header[first_byte]=a;
 header[second_byte]=b;
}

void makeHeader(unsigned char* header, struct rudp_header header_info){
 //unsigned char header[9];
 insertBytes(header_info.eof, header,0,1);
 insertBytes(header_info.seq_no, header,2,3);
 insertBytes(header_info.ack, header, 4, 4);
 insertBytes(header_info.ack_no,header, 5, 6);
 insertBytes(header_info.data_length,header, 7,8);
 //return header;
}

int getBytes(unsigned char* bytes,int first_byte_pos, int second_byte_pos){
 int c, d;
 c = bytes[first_byte_pos];
 // if(c==255)
 //  c=0;
 //printf("\n first byte %d", c);
 if(first_byte_pos == second_byte_pos)
  return c;
 c=c<<8;
 d = bytes[second_byte_pos];
 // if(d==255)
 //  d=0; 
 c = c|(d);
 //printf("\n second byte %d and result %d\n",d,c);
 return c;
}

struct rudp_header getHeaderInfo(char *bytes){
 struct rudp_header header_info;
 header_info.eof = getBytes(bytes, 0, 1);
 header_info.seq_no = getBytes(bytes, 2, 3);
 header_info.ack = getBytes(bytes, 4, 4);
 header_info.ack_no = getBytes(bytes,5, 6);
 header_info.data_length = getBytes(bytes, 7, 8);
 return header_info;
}

int nextBool(double probability)
{
  return (rand() >  probability * (double)RAND_MAX) ? -1 : 0;
}


#endif

