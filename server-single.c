#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <arpa/inet.h>

typedef struct GAME {
   int nMoves;      // total number of valid moves
   int gameOver;    // 0: not over, 1: a winner, 2: a tie
   char stone;      // 'W': for white stone, 'B' for black
   int x, y;        // (x, y): current move
   char board[8][8];
} Game;


#define BUFFERSIZE 256 

int get_server_connection(char *hostname, char *port);
void compose_http_request(char *http_request, char *filename);
void web_browser(int http_conn);
void print_ip( struct addrinfo *ai);
void printArray(char ptr[][8]);


int main(int argc, char *argv[])
{
    int http_conn;  
    char http_request[BUFFERSIZE];

    if (argc != 4) {
        printf("usage: client HOST HTTPORT webpage\n");
        exit(1);
    }

    // steps 1, 2: get a connection to server
    if ((http_conn = get_server_connection(argv[1], argv[2])) == -1) {
       printf("connection error\n");
       exit(1);
    }

    // step 3: web browser, send to/receive from the server
    web_browser(http_conn);

    // step 4: as always, close the socket when done
    close(http_conn);
}

int get_server_connection(char *hostname, char *port) {
    int serverfd;
    struct addrinfo hints, *servinfo, *p;
    int status;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

   if ((status = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
       printf("getaddrinfo: %s\n", gai_strerror(status));
       return -1;
    }

    print_ip(servinfo);
    for (p = servinfo; p != NULL; p = p ->ai_next) {
       // Step 1: get a socket
       if ((serverfd = socket(p->ai_family, p->ai_socktype,
                           p->ai_protocol)) == -1) {
           printf("socket socket \n");
           continue;
       }

       // Step 2: connect to the server
       if ((status = connect(serverfd, p->ai_addr, p->ai_addrlen)) == -1) {
           close(serverfd);
           printf("socket connect \n");
           continue;
       }

       break;
    }

    freeaddrinfo(servinfo);
   
    if (status != -1) return serverfd;
    else return -1;
}

//once a connection has been made, this method
//sends things to and recieves things from the
///server
void web_browser(int http_conn) {

   //placeholders for potential data
   //sent from the server
   char msg2[100];
   Game *game;
   int x,y;
   int type;
   int gameOver = 1;
   game = (Game *) malloc(sizeof(Game));

   // step 3.2: receive message from server
   while(gameOver == 1){
     
      //get the type of message
      recv(http_conn, &type, sizeof(int), 0);
      
      if(type == 1){
         recv(http_conn, game, sizeof(Game), 0);
         printArray(game->board);
      }

      if(type == 2){
         recv(http_conn, msg2, sizeof(msg2), 0);
         printf("%s", msg2);
         scanf("%d%d", &x, &y);
         send(http_conn, &x, sizeof(int), 0);
         send(http_conn, &y, sizeof(int), 0);
      }

      if(type == 3){
         recv(http_conn, msg2, sizeof(msg2), 0);
         printf("%s", msg2);
         gameOver = 0;
      }
      
   }
}

void print_ip( struct addrinfo *ai) {
   struct addrinfo *p;
   void *addr;
   char *ipver;
   char ipstr[INET6_ADDRSTRLEN];
   struct sockaddr_in *ipv4;
   struct sockaddr_in6 *ipv6;
   short port = 0;

   for (p = ai; p !=  NULL; p = p->ai_next) {
      if (p->ai_family == AF_INET) {
         ipv4 = (struct sockaddr_in *)p->ai_addr;
         addr = &(ipv4->sin_addr);
         port = ipv4->sin_port;
         ipver = "IPV4";
      }
      else {
         ipv6= (struct sockaddr_in6 *)p->ai_addr;
         addr = &(ipv6->sin6_addr);
         port = ipv4->sin_port;
         ipver = "IPV6";
      }
      inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
      printf("serv ip info: %s - %s @%d\n", ipstr, ipver, ntohs(port));
   }
}

//prints the board
void printArray(char ptr[][8]){
   int i, j;
   for (i=0; i < 8; i++) {
      for (j=0; j < 8; j++) {
         printf("%c", ptr[i][j]);
         if(i<8 && j<7){
            printf(" | ");
         }
      }
      if (j==8 && i<7){
         printf("\n");
         printf("--+---+---+---+---+---+---+--");
         printf("\n");
      }
   }
   printf("\n");
}

