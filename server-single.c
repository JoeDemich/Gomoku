#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <netdb.h>
#include <pthread.h>

typedef struct GAME {
   int nMoves;      // total number of valid moves
   int gameOver;    // 0: not over, 1: a winner, 2: a tie
   char stone;      // 'W': for white stone, 'B' for black
   int x, y;        // (x, y): current move
   char board[8][8];
} Game;

//struct that contains a game and the ports of 
//the two players
typedef struct NEWGAME {

   int p1Sock;
   int p2Sock;
   Game* oldGame; 

} newGame;


//initialize methods
void fillBoard(char ptr[][8]);
void printArray(char ptr[][8]);
void *horizontalCheck(void *ptr);
void *verticalCheck(void *ptr);
void getMove(int p_sock_fd, Game *game);


// you must uncomment the next two lines and make changes
#define HOST "freebsd2.cs.scranton.edu" // the hostname of the HTTP server
#define HTTPPORT "17603"               // the HTTP port client will be connecting to
#define BACKLOG 10                     // how many pending connections queue will hold

void *get_in_addr(struct sockaddr * sa);             // get internet address
int get_server_socket(char *host, char *port);   // get a server socket
int start_server(int serv_socket, int backlog);      // start server's listening
int accept_client(int serv_sock);                    // accept a connection from client
void start_subserver(newGame* game);             // start subserver as a thread
void *subserver(void* game);            // subserver - runs the game
void print_ip( struct addrinfo *ai);                 // print IP info from getaddrinfo()

int main(void)
{
    int http_sock_fd;			// http server socket
    int reply_sock_fd1;  		// client connection
    int reply_sock_fd2; 
    int yes;

    // steps 1-2: get a socket and bind to ip address and port
    http_sock_fd = get_server_socket(HOST, HTTPPORT);

    // step 3: get ready to accept connections
    if (start_server(http_sock_fd, BACKLOG) == -1) {
       printf("start server error\n");
       exit(1);
    }

    while(1) {  // accept() client connections
        // step 4: accept a client connection
        // each client is assigned a unique reply_sock_fd, that
        // can be used to communicate with the client.

        if ((reply_sock_fd1 = accept_client(http_sock_fd)) == -1) {
            continue;
        }
        if ((reply_sock_fd2 = accept_client(http_sock_fd)) == -1) {
            continue;
        }
        newGame *g = (newGame *)malloc(sizeof(newGame));
        g->p1Sock = reply_sock_fd1;
        g->p2Sock = reply_sock_fd2;
        // steps 5, 6, 7: read from and write to sockets, close socket
        start_subserver(g);
    }
} 

int get_server_socket(char *host, char *port) {
    struct addrinfo hints, *servinfo, *p;
    int status;
    int server_socket;
    int yes = 1;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(host, port, &hints, &servinfo)) != 0) {
       printf("getaddrinfo: %s\n", gai_strerror(status));
       exit(1);
    }

    for (p = servinfo; p != NULL; p = p ->ai_next) {
       // step 1: create a socket
       if ((server_socket = socket(p->ai_family, p->ai_socktype,
                           p->ai_protocol)) == -1) {
           printf("socket socket \n");
           continue;
       }
       // if the port is not released yet, reuse it.
       if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
         printf("socket option\n");
         continue;
       }

       // step 2: bind socket to an IP addr and port
       if (bind(server_socket, p->ai_addr, p->ai_addrlen) == -1) {
           printf("socket bind \n");
           continue;
       }
       break;
    }
    print_ip(servinfo);
    freeaddrinfo(servinfo);   // servinfo structure is no longer needed. free it.

    return server_socket;
}

int start_server(int serv_socket, int backlog) {
    int status = 0;
    if ((status = listen(serv_socket, backlog)) == -1) {
        printf("socket listen error\n");
    }
    return status;
}

//accept a client connection
int accept_client(int serv_sock) {
    int reply_sock_fd = -1;
    socklen_t sin_size = sizeof(struct sockaddr_storage);
    struct sockaddr_storage client_addr;
    char client_printable_addr[INET6_ADDRSTRLEN];

    // accept a connection request from a client
    // the returned file descriptor from accept will be used
    // to communicate with this client.
    if ((reply_sock_fd = accept(serv_sock, 
       (struct sockaddr *)&client_addr, &sin_size)) == -1) {
            printf("socket accept error\n");
    }
    else {
        // here is for info only, not really needed.
        inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr), 
                          client_printable_addr, sizeof client_printable_addr);
        printf("server: connection from %s at port %d\n", client_printable_addr,
                            ((struct sockaddr_in*)&client_addr)->sin_port);
    }
    return reply_sock_fd;
}

// this is a place holder for future thread-based implementation
void start_subserver(newGame* game) {
   subserver( (void *) game);
}

//this is the subserver who really communicate with client through the 
//socks of player 1 and 2 provided by newGame.  
//this will be executed as a thread in the future. 
void *subserver(void * g) {

   //create a pointer to a game struct that will be used
   newGame *newG = (newGame *) g;
   newG->oldGame = (Game *)malloc(sizeof(Game)); 
   Game *game = newG->oldGame;
   pthread_t thread1, thread2;  //threads intialized
   game->gameOver = 0;
   game->nMoves = 0;
   //fills the board with '-'
   fillBoard(game->board);
   //used to hold what will be sent
   int type;
   char msg2[100];
   //the central loop that continues until
   //either one player wins or the board fills
   while(game->gameOver == 0){

      int currentPlayer;
 
      //determines turn
      if(game->nMoves %2 == 0){
         currentPlayer = newG->p1Sock;
         game->stone = 'B';
         strcpy(msg2,
         "It's player 1's turn. Enter the coordinates of your move seperated by a space");

      }

      else{
         currentPlayer = newG->p2Sock;
         game->stone = 'W';
         strcpy(msg2,
         "It's player 2's turn. Enter the coordinates of your move seperated by a space");

      }
      //send the type of message then the corresponding data
      //1: game board
      //2: prompt for move/if an invalid move was recieved
      //3: if the game has ended, tell the users
      type = 1;
      send(currentPlayer, &type, sizeof(int), 0);
      send(currentPlayer, game, sizeof(Game), 0);
      type = 2;
      send(currentPlayer, &type, sizeof(int), 0);
      send(currentPlayer, msg2, sizeof(msg2), 0);

      getMove(currentPlayer, game);

      //this loop is entered if the move that was entered is
      //either out of bounds or trying to place a stone
      //over another stone
      while(((game->x > 7 || game->x < 0) || (game->y < 0 || game->y > 7))
         || (game->board[game->x][game->y] != '-')){
         if ((game->x > 7 || game->x < 0) || (game->y < 0 || game->y > 7)){
            sprintf(msg2, "(%d, %d) is outside the scale of the board, try again.\n", 
               game->x, game->y);
            type = 2;
            send(currentPlayer, &type, sizeof(int), 0);
            send(currentPlayer, msg2, sizeof(msg2), 0);

            getMove(currentPlayer, game);
         }
         else if (game->board[game->x][game->y] != '-'){
            sprintf(msg2, "(%d, %d) is already full, try again.\n",
               game->x, game->y);
            type = 2;
            send(currentPlayer, &type, sizeof(int), 0);
            send(currentPlayer, msg2, sizeof(msg2), 0);

            getMove(currentPlayer, game);
         }
      }

      //places the stone
      game->board[game->x][game->y] = game->stone;

      //create and start the two threads, calling the
      //horizontal and vertical checks concurrently
      pthread_create( &thread1, NULL, horizontalCheck, (void*) newG);
      pthread_create( &thread2, NULL, verticalCheck, (void*) newG);

      pthread_join( thread1, NULL);
      pthread_join( thread2, NULL);

      //if the board is filled, end in a tie
      game->nMoves++;
      if(game->nMoves == 64){
         game->gameOver = 2;

         type = 3;      
         strcpy(msg2, "It's a tie!");
         send(newG->p1Sock, &type, sizeof(int), 0);
         send(newG->p2Sock, &type, sizeof(int), 0);
         send(newG->p1Sock, msg2, sizeof(msg2), 0);
         send(newG->p2Sock, msg2, sizeof(msg2), 0);

      }

   }
   
   close(newG->p1Sock);
   close(newG->p2Sock);

   return NULL;
}

//checks if the amount of 'B' or 'W' has
//reached or gone over 5
//not much has changed accept that instead of 
//printing who won, it sends a winning message
void *horizontalCheck(void *ptr) {
   newGame *game = (newGame *)ptr;
   int i, j;
   char msg[20];
   int type;
   int bHoriCheck = 0;
   int wHoriCheck = 0;
   for (i=0; i < 8; i++) {
      for (j=0; j < 8; j++) {
         if(game->oldGame->board[i][j] == 'B'){
            bHoriCheck++;   //if value is B increment
            wHoriCheck = 0; //reset W check since the streak is over
            if(bHoriCheck == 5){
               game->oldGame->gameOver = 1;
               strcpy(msg, "Player 1 wins!\n");
               type = 3;
               send(game->p1Sock, &type, sizeof(int), 0);
               send(game->p1Sock, msg, sizeof(msg), 0);
               send(game->p2Sock, &type, sizeof(int), 0);
               send(game->p2Sock, msg, sizeof(msg), 0);

               return 0;
            }
         }
         else if(game->oldGame->board[i][j] == 'W'){
            wHoriCheck++;   //if value is W increment
            bHoriCheck = 0; //reset B check since the streak is over
            if(wHoriCheck == 5){
               game->oldGame->gameOver = 1;
               strcpy(msg, "Player 2 wins!\n");
               type = 3;
               send(game->p1Sock, &type, sizeof(int), 0);
               send(game->p1Sock, msg, sizeof(msg), 0);
               send(game->p2Sock, &type, sizeof(int), 0);
               send(game->p2Sock, msg, sizeof(msg), 0);

               return 0;
           }
         }
         //if we've made it here the current value
         //at the coordinates must be '-'
         //so both streaks must be reset
         else{
            bHoriCheck = 0;
            wHoriCheck = 0;
         }
      }
      bHoriCheck = 0;
      wHoriCheck = 0;
   }
   return 0;
}

void *verticalCheck(void *ptr) {
   newGame *game = (newGame *)ptr;
   int i, j;
   char msg[20];
   int type;
   int bVertCheck = 0;
   int wVertCheck = 0;
   for (i=0; i < 8; i++) {
      for (j=0; j < 8; j++) {
         if(game->oldGame->board[j][i] == 'B'){
            bVertCheck++;   //if value is B increment
            wVertCheck = 0; //reset W check since the streak is over
            if(bVertCheck == 5){
               game->oldGame->gameOver = 1;
               strcpy(msg, "Player 1 wins!\n");
               type = 3;
               send(game->p1Sock, &type, sizeof(int), 0);
               send(game->p1Sock, msg, sizeof(msg), 0);
               send(game->p2Sock, &type, sizeof(int), 0);
               send(game->p2Sock, msg, sizeof(msg), 0);

               return 0;
            }
         }
         else if(game->oldGame->board[j][i] == 'W'){
            wVertCheck++;   //if value is W increment
            bVertCheck = 0; //reset B check since the streak is over
            if(wVertCheck == 5){
               game->oldGame->gameOver = 1;
               strcpy(msg, "Player 2 wins!\n");
               type = 3;
               send(game->p1Sock, &type, sizeof(int), 0);
               send(game->p1Sock, msg, sizeof(msg), 0);
               send(game->p2Sock, &type, sizeof(int), 0);
               send(game->p2Sock, msg, sizeof(msg), 0);

               return 0;
            }
         }
         //if we've made it here the current value
         //at the coordinates must be '-'
         //so both streaks must be reset
         else{
            wVertCheck = 0;
            bVertCheck = 0;
         }
      }
      bVertCheck = 0;
      wVertCheck = 0;
   }
   return 0;
}

//used to initially fill board with '-'
void fillBoard(char ptr[][8]){
   memset(ptr, '-', sizeof(ptr[0][0]) * 8 * 8);
}

//reads a player's move
void getMove(int p_sock_fd, Game *game){
   
   int x,y;
   //receive the coordinates
   recv(p_sock_fd, &x, sizeof(int), 0);
   recv(p_sock_fd, &y, sizeof(int), 0);

   game->x = x;
   game->y = y;

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

// ======= HELP FUNCTIONS =========== //
/* the following is a function designed for testing.
   it prints the ip address and port returned from
   getaddrinfo() function */
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

void *get_in_addr(struct sockaddr * sa) {
   if (sa->sa_family == AF_INET) {
      printf("ipv4\n");
      return &(((struct sockaddr_in *)sa)->sin_addr);
   }
   else {
      printf("ipv6\n");
      return &(((struct sockaddr_in6 *)sa)->sin6_addr);
   }
}
