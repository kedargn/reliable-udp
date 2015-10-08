#include<stdio.h>
#include<stdlib.h>

struct stack{
	int elements[100];
	int head;
};

int n;
struct stack S;

/**
 * Pushes the element to stack and updates the head of the stack
 * if stack is full, appropriate error message is printed
 */
void push(int element){
	if(S.head == n-1){
    printf("\n\n\n\nSTACK OVERFLOW\n\n\n\n");
	} else {
		S.head = S.head+1;
		S.elements[S.head] = element;
		printf("%d pushed to stack. head is at %d\n ", element, S.head);
	}
}

/**
 * pops the element at the head of the stack.
 * @return: -1 if the stack is empty
 *           popped elements value
 */ 

int pop(){
	int element;
  if(S.head == -1){
  	printf("\n\n\n\nSTACK UNDERFLOW\n\n\n\n");
  	return -1;
  } else {
  	element = S.elements[S.head--];
  	printf("Element %d popped from stack. Head is at %d\n", element, S.head);
  	return element;
  }
}


/** Displays contents of stack starting from head of the stack
  * If stack is empty appropriate message is printed
  */
void display(){
	int i;
	if(S.head == -1){
  	printf("STACK IS EMPTY\n");
  } else {
  	i = S.head;
  	printf("Printing Stack\n\n\n\n\n");
  	for(;i>=0;i--){
  		printf("%d\n", S.elements[i]);
  	}
  }
}

void main(){
	int i, element, choice, result;
	S.head = -1;
	printf("Enter the number of elements\n");
	scanf("%d", &n);
	for(;;){
		i++;
    printf("Enter\n1.Push\n2.Pop\n3.Display\n4.Exit\n");
    printf("Enter your choice\n");
    scanf("%d", &choice);
    switch(choice){
    	case 1:printf("Enter element to be pushed\n");
    	       scanf("%d",&element);
    	       push(element);
             break;
      case 2:result = pop();
             break;
      case 3:display();
             break;
      case 4:exit(0);
      default: printf("Invalid Input\n");
    }
  }
}