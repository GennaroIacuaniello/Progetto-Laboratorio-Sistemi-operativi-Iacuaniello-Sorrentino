//Compile: gcc server.c -o server.out -lpthread
//Run: ./server.out

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>


#define PORT    8080
#define BACKLOG 16

#define MAX_PLAYERS_MATCH 4
#define SECONDS_PER_SIZE 120        //Match lenght starts from 2 minutes (120 seconds) and scales with the map size (16x16 120 sec, 32x32 240 sec, 48x48 360 sec)
#define TIMER_GLOBAL_MAP 30         //Interval of seconds every time the server sends the global map

/*
lista giocatori: 

lista (nodo: nickname, lobby in cui è)

mappa: 

matrice (di interi, vedi sotto) di dimensione a scelta dell'utente (16x16, 32x32, 48x48, 64x64); 

cella: 

UN INTERO gestito come segue:

000...01 muro
000...00 cella normale

      3210
[0,0] 0010 <-- cella normale colorata da giocatore 1
[1,0] 1010 <-- cella normale colorata da giocatore 1 e occupata da giocatore 1
[1,1] 0111 <-- muro scoperto da entrambi i giocatori;

per partita, array (o lista?) informazione giocatore:

cella (o nodo?): posizione_x, posizione_y, num celle colorate


mappe: 4 preset 16x16 scelti casualmente dal server (forse?)


*/

typedef struct User{

      char username[64];
      unsigned long hashed_password;

}User;

/*
typedef struct User_list_node{

      unsigned char username[64];
      unsigned int id_lobby;
      struct User_list_node* next;

}User_list_node;

typedef struct Maps_list_node{

      unsigned int** map;
      unsigned int id_lobby;
      struct Maps_list_node* next;

}Maps_list_node;
*/

typedef struct User_in_match{

      char username[64];
      pthread_t tid;                //tid of the thread of the user, used to send signals
      unsigned int x;
      unsigned int y;
      //unsigned int num_colored_cells;

}User_in_match;

typedef enum {
    IN_PROGRESS,
    NOT_IN_PROGRESS
} Match_status;

typedef struct Match{         //Colors of the game with 4 players: players[0] = blue, players[1] = red, players[2] = green, players[3] = yellow

      uint32_t id;                          //id of the lobby
      User_in_match* players[MAX_PLAYERS_MATCH];      //array of players, up to MAX_PLAYERS_MATCH players per match
      int host;                                       //Index of the array players that indicates which one is the host of the game
      
      Match_status status;                       //status of the match (IN_PROGESS or NOT_IN_PROGESS)
      
      uint32_t size;                                       //Map size
      char** map;                                     //Map of the game

      pthread_mutex_t match_mutex;                    //used for: players, host, status, size, map
      pthread_cond_t match_cond_var;    
      int match_free;

}Match;

typedef struct Match_list_node{

      Match* match;
      struct Match_list_node* next;

}Match_list_node;

//Global list of matches
Match_list_node* match_list = NULL;

typedef struct Message_with_local_information{

      uint32_t players_position[MAX_PLAYERS_MATCH][2];        //Every row has 2 cells, if a player is in the local map, its cells are not size of the map + 1

      uint32_t receiver_id;                                   //Id_in_match of the player who is receiving the message, used to know its data from the array players_position 

      char local_map[5][5];

      char message[25];

}Message_with_local_information;

typedef struct Global_Info_Header{
      
      uint32_t players_position[MAX_PLAYERS_MATCH][2];      //Every row has 2 cells, if a player is in the global map, its cells are not size of the map + 1

      uint32_t receiver_id;                                   //Id_in_match of the player who is receiving the message, used to know its data from the array players_position 

      uint32_t size;                                        //Global map size

} Global_Info_Header;

const unsigned char colors[MAX_PLAYERS_MATCH] = {'b', 'r', 'g', 'y'};      //Global variable used for player colors

//Variables used to give ids to matches
pthread_mutex_t next_match_id_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t next_match_id_cond_var = PTHREAD_COND_INITIALIZER;    
int next_match_id_free = 1;

uint32_t next_match_id = 1;

//pthread_key_t key_for_socket;       //variable used for thread_specific_data for the socket associated to the thread


//Variables used as maps pre-sets
/*
const unsigned char map_preset_1[16][16] = {
      {'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e'},
      {'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'w', 'e'},
      {'e', 'e', 'w', 'e', 'w', 'e', 'w', 'e', 'w', 'e', 'w', 'e', 'w', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e'},
      {'e', 'e', 'w', 'e', 'w', 'e', 'e', 'e', 'w', 'e', 'e', 'w', 'e', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e'},
      {'e', 'e', 'w', 'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'w', 'e', 'w', 'w', 'w'},
      {'w', 'w', 'w', 'e', 'w', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e', 'w', 'e', 'e'},
      {'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'e', 'w', 'e', 'e', 'w', 'e', 'e', 'e', 'w', 'e', 'w', 'e', 'e'},
      {'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'w', 'e', 'w', 'e', 'w', 'e', 'w', 'e', 'w', 'e', 'w', 'e', 'e'},
      {'e', 'w', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e'},
      {'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e'}
};

const unsigned char map_preset_2[16][16] = {
      {'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e'},
      {'e', 'w', 'e', 'e', 'e', 'e', 'e', 'w', 'w', 'e', 'e', 'e', 'e', 'e', 'w', 'e'},
      {'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e'},
      {'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'w', 'w', 'w', 'e', 'e', 'e', 'e', 'w', 'w', 'w', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'e', 'e', 'w', 'e', 'w', 'w', 'e', 'w', 'e', 'e', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'e', 'e', 'w', 'e', 'w', 'w', 'e', 'w', 'e', 'e', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'w', 'w', 'w', 'e', 'e', 'e', 'e', 'w', 'w', 'w', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e'},
      {'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e'},
      {'e', 'w', 'e', 'e', 'e', 'e', 'e', 'w', 'w', 'e', 'e', 'e', 'e', 'e', 'w', 'e'},
      {'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e'}
};

const unsigned char map_preset_3[16][16] = {
      {'e', 'e', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'e', 'e', 'e', 'e', 'e', 'e'},
      {'w', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'w', 'w', 'e', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'w', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'e', 'w', 'w', 'w', 'w', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e'},
      {'e', 'e', 'e', 'w', 'e', 'e', 'e', 'w', 'w', 'w', 'e', 'e', 'e', 'e', 'w', 'e'},
      {'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e', 'w', 'e'},
      {'e', 'e', 'w', 'e', 'e', 'e', 'w', 'w', 'e', 'e', 'e', 'w', 'e', 'e', 'w', 'e'},
      {'e', 'w', 'w', 'e', 'e', 'w', 'e', 'e', 'w', 'e', 'e', 'w', 'e', 'e', 'w', 'e'},
      {'e', 'w', 'e', 'e', 'w', 'e', 'e', 'e', 'w', 'e', 'e', 'w', 'e', 'e', 'w', 'e'},
      {'e', 'w', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'w', 'e'},
      {'e', 'w', 'e', 'e', 'e', 'w', 'w', 'e', 'e', 'e', 'w', 'e', 'e', 'e', 'w', 'e'},
      {'e', 'w', 'w', 'e', 'e', 'e', 'w', 'w', 'w', 'w', 'e', 'e', 'e', 'w', 'w', 'e'},
      {'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e'},
      {'e', 'e', 'e', 'w', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'w', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'e', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'e', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e'}
};

const unsigned char map_preset_4[16][16] = {
      {'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'w', 'e', 'w', 'e', 'w', 'w', 'w', 'e', 'w', 'e', 'w', 'e', 'e'},
      {'e', 'e', 'e', 'w', 'e', 'e', 'e', 'w', 'e', 'w', 'e', 'e', 'e', 'w', 'e', 'e'},
      {'e', 'e', 'e', 'w', 'e', 'e', 'w', 'e', 'e', 'e', 'w', 'e', 'e', 'w', 'e', 'e'},
      {'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e'},
      {'e', 'e', 'w', 'e', 'e', 'w', 'w', 'e', 'e', 'e', 'w', 'w', 'e', 'e', 'w', 'e'},
      {'e', 'w', 'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'w'},
      {'e', 'w', 'e', 'e', 'e', 'w', 'w', 'e', 'e', 'e', 'w', 'w', 'e', 'e', 'e', 'w'},
      {'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w'},
      {'e', 'w', 'e', 'e', 'e', 'e', 'e', 'w', 'w', 'w', 'e', 'e', 'e', 'e', 'e', 'w'},
      {'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'w'},
      {'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'w'},
      {'e', 'w', 'e', 'e', 'e', 'w', 'e', 'e', 'w', 'e', 'e', 'w', 'e', 'e', 'e', 'w'},
      {'e', 'e', 'w', 'e', 'e', 'e', 'w', 'w', 'e', 'w', 'w', 'e', 'e', 'e', 'w', 'e'},
      {'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'e', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'e', 'e', 'e'}
};

*/
const unsigned char array_of_map_presets[4][16][16] = {

      {
      {'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e'},
      {'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'w', 'e'},
      {'e', 'e', 'w', 'e', 'w', 'e', 'w', 'e', 'w', 'e', 'w', 'e', 'w', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e'},
      {'e', 'e', 'w', 'e', 'w', 'e', 'e', 'e', 'w', 'e', 'e', 'w', 'e', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e'},
      {'e', 'e', 'w', 'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'w', 'e', 'w', 'w', 'w'},
      {'w', 'w', 'w', 'e', 'w', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e', 'w', 'e', 'e'},
      {'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'e', 'w', 'e', 'e', 'w', 'e', 'e', 'e', 'w', 'e', 'w', 'e', 'e'},
      {'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'w', 'e', 'w', 'e', 'w', 'e', 'w', 'e', 'w', 'e', 'w', 'e', 'e'},
      {'e', 'w', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e'},
      {'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e'}
      },

      {
      {'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e'},
      {'e', 'w', 'e', 'e', 'e', 'e', 'e', 'w', 'w', 'e', 'e', 'e', 'e', 'e', 'w', 'e'},
      {'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e'},
      {'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'w', 'w', 'w', 'e', 'e', 'e', 'e', 'w', 'w', 'w', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'e', 'e', 'w', 'e', 'w', 'w', 'e', 'w', 'e', 'e', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'e', 'e', 'w', 'e', 'w', 'w', 'e', 'w', 'e', 'e', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'w', 'w', 'w', 'e', 'e', 'e', 'e', 'w', 'w', 'w', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e'},
      {'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e'},
      {'e', 'w', 'e', 'e', 'e', 'e', 'e', 'w', 'w', 'e', 'e', 'e', 'e', 'e', 'w', 'e'},
      {'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e'}
      },

      {
      {'e', 'e', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'e', 'e', 'e', 'e', 'e', 'e'},
      {'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'w', 'w', 'e', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'w', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'e', 'w', 'w', 'w', 'w', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e'},
      {'e', 'e', 'e', 'w', 'e', 'e', 'e', 'w', 'w', 'w', 'e', 'e', 'e', 'e', 'w', 'e'},
      {'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e', 'w', 'e'},
      {'e', 'e', 'w', 'e', 'e', 'e', 'w', 'w', 'e', 'e', 'e', 'w', 'e', 'e', 'w', 'e'},
      {'e', 'w', 'w', 'e', 'e', 'w', 'e', 'e', 'w', 'e', 'e', 'w', 'e', 'e', 'w', 'e'},
      {'e', 'w', 'e', 'e', 'w', 'e', 'e', 'e', 'w', 'e', 'e', 'w', 'e', 'e', 'w', 'e'},
      {'e', 'w', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'w', 'e'},
      {'e', 'w', 'e', 'e', 'e', 'w', 'w', 'e', 'e', 'e', 'w', 'e', 'e', 'e', 'w', 'e'},
      {'e', 'w', 'w', 'e', 'e', 'e', 'w', 'w', 'w', 'w', 'e', 'e', 'e', 'w', 'w', 'e'},
      {'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e'},
      {'e', 'e', 'e', 'w', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'w', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'e', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'e', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e'}
      },

      {
      {'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'w', 'e', 'w', 'e', 'w', 'w', 'w', 'e', 'w', 'e', 'w', 'e', 'e'},
      {'e', 'e', 'e', 'w', 'e', 'e', 'e', 'w', 'e', 'w', 'e', 'e', 'e', 'w', 'e', 'e'},
      {'e', 'e', 'e', 'w', 'e', 'e', 'w', 'e', 'e', 'e', 'w', 'e', 'e', 'w', 'e', 'e'},
      {'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e'},
      {'e', 'e', 'w', 'e', 'e', 'w', 'w', 'e', 'e', 'e', 'w', 'w', 'e', 'e', 'w', 'e'},
      {'e', 'w', 'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e'},
      {'e', 'w', 'e', 'e', 'e', 'w', 'w', 'e', 'e', 'e', 'w', 'w', 'e', 'e', 'e', 'w'},
      {'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'w'},
      {'e', 'w', 'e', 'e', 'e', 'e', 'e', 'w', 'w', 'w', 'e', 'e', 'e', 'e', 'e', 'w'},
      {'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'w'},
      {'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'w', 'e', 'e', 'e', 'e', 'e', 'e', 'w'},
      {'e', 'w', 'e', 'e', 'e', 'w', 'e', 'e', 'w', 'e', 'e', 'w', 'e', 'e', 'e', 'w'},
      {'e', 'e', 'w', 'e', 'e', 'e', 'w', 'w', 'e', 'w', 'w', 'e', 'e', 'e', 'w', 'e'},
      {'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e', 'e'},
      {'e', 'e', 'e', 'e', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'e', 'e', 'e'}
      }

};

//Variables used to control access to users file "Database" (in contains couples of data username-hashed_password)
pthread_mutex_t users_file_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t users_file_cond_var = PTHREAD_COND_INITIALIZER;    
int users_file_free = 1;


User* handle_starting_interaction(int socket_for_thread);
User* handle_login(int socket_for_thread);
User* handle_registration(int socket_for_thread);


unsigned int login(char* username, char* password);
unsigned int registration(char* username, char* password);
unsigned long hash(unsigned char *str);


void handle_session(int socket_for_thread, User* current_user);
void lobby_creation(int socket_for_thread, User* current_user);
void join_lobby(int socket_for_thread, User* current_user);
char* get_message_with_matches_info(int socket_for_thread, int error_flag);
unsigned int join_spcific_lobby(int socket_for_thread, User* current_user, Match_list_node* match_to_join);

void handle_being_in_lobby(int socket_for_thread, Match_list_node* match_node, User* current_user, int id_in_match);
char* get_message_list_of_players_and_size(int socket_for_thread, User* current_user, Match_list_node* match_node, int id_in_match);

void start_match(int socket_for_thread, Match_list_node* match_node, User* current_user, int id_in_match);
void change_map_size(int socket_for_thread, Match_list_node* match_node, User* current_user, int id_in_match);

void handle_leaving_lobby(Match_list_node* match_node, int id_in_match);
void check_host_change_or_match_cancellation(Match_list_node* match_node, int previous_host_id_in_match);
void handle_client_death_in_lobby(int socket_for_thread, Match_list_node* match_node, User* current_user, int id_in_match);


void handle_being_in_match(int socket_for_thread, Match_list_node* match_node, User* current_user, int id_in_match);
Message_with_local_information* get_message_with_local_information(Match_list_node* match_node, int id_in_match);
void send_global_map_to_client(int socket_for_thread, Match_list_node* match_node, User* current_user, int id_in_match);
void handle_move_in_match(Match_list_node* match_node, int id_in_match, char option);
void move(Match_list_node* match_node, int id_in_match, int new_x, int new_y);
void handle_match_ending(int socket_for_thread, Match_list_node* match_node, User* current_user, int id_in_match);

Match* init_match(int socket_for_thread, User* creator, uint32_t size);
void free_match(Match* match);

ssize_t send_all(int socket_for_thread, const void* buf, size_t n);
ssize_t recv_all(int socket_for_thread, void* buf, size_t n, int flags);
ssize_t recv_string(int socket_for_thread, char* buf, size_t max_len, int flags);

ssize_t send_all_in_match(int socket_for_thread, const void* buf, size_t n, Match_list_node* match_node, User* current_user, int id_in_match);
ssize_t recv_all_in_match(int socket_for_thread, void* buf, size_t n, int flags, Match_list_node* match_node, User* current_user, int id_in_match);



static void* handle_client(void* arg);


//Handler of the signal SIGUSR1 used for the start of the matches
//void siguser1_handler(int signal);
//TODO Rimuovere le seguenti funzioni di debug
/*void read_users(){
      
      FILE* file_users = fopen("users.dat", "rb+");

      if(file_users != NULL){
            
            User current_user;

            while(fread(&current_user, sizeof(User), 1, file_users) == 1){

                  printf("%s, %ld", current_user.username, current_user.hashed_password);
                        

            }

            
      }else{
            perror("fopen");
            
      }

}*/


int main(int argc, char* argv[]){

      signal(SIGPIPE, SIG_IGN);
      //signal(SIGUSR1, siguser1_handler);

      srand(time(NULL));

      //pthread_key_create(&key_for_socket, NULL);

      //se non esiste già, crea il file "database" di utenti
      int fd = open("users.dat", O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
      if(fd >= 0)
            close(fd);
      else{
            perror("open"); 
            exit(1);
      }

      /* TODO rimuovere
      int res = login("admin", "admin");
      
      printf("%d\n", res);
      read_users();
      */

      int listen_socket;
      struct sockaddr_in server_addr;

      listen_socket = socket(AF_INET, SOCK_STREAM, 0);
      if (listen_socket < 0) { 
            perror("socket"); 
            exit(1); 
      }

      int on = 1;
      setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

      memset(&server_addr, 0, sizeof(server_addr));
      server_addr.sin_family      = AF_INET;
      server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
      server_addr.sin_port        = htons(PORT);

      if (bind(listen_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            perror("bind"); 
            exit(1);
      }

      if (listen(listen_socket, 1) < 0) {
            perror("listen"); 
            exit(1);
      }

      printf("Server in ascolto su porta %d...\n", PORT);

      while(1){   


            int connection_socket = accept(listen_socket, NULL, NULL);
            if (connection_socket < 0) { 

                  if (errno == EINTR) 
                        continue; 
                  
                  perror("accept"); 
                  continue; 
            }

            printf("Client connesso su connection socket %d", connection_socket);
	
            int* socket_for_thread =(int*) malloc(sizeof(int));

            if (!socket_for_thread) {

                  perror("malloc");
                  close(connection_socket);

                  continue;

            }

            pthread_t handle_thread;

            *socket_for_thread = connection_socket;
            
            int err = pthread_create(&handle_thread, NULL, handle_client, (void*)socket_for_thread);

            if (err != 0) {
                  fprintf(stderr, "pthread_create: %s\n", strerror(err));
                  close(connection_socket);
                  free(socket_for_thread);
                  continue;
            }

            pthread_detach(handle_thread);

            

      }

      close(listen_socket);

      //pthread_key_delete(key_for_socket);

      return 0;
}










unsigned int login(char* username, char* password){


      pthread_mutex_lock(&users_file_mutex);

      while(users_file_free == 0)
            pthread_cond_wait(&users_file_cond_var, &users_file_mutex);

      users_file_free = 0;

      FILE* file_users = fopen("users.dat", "rb");
      unsigned long hashed_password = hash((unsigned char*)password);

      if(file_users != NULL){
            
            User current_user;

            while(fread(&current_user, sizeof(User), 1, file_users) == 1){

                  if( ( strcmp(username, current_user.username) == 0 ) &&
                        ( hashed_password == current_user.hashed_password ) ){
                              
                              fclose(file_users);

                              users_file_free = 1;
                              pthread_cond_signal(&users_file_cond_var);
                              pthread_mutex_unlock(&users_file_mutex);

                              return 0;

                        }
                        

            }
            fclose(file_users);

            users_file_free = 1;
            pthread_cond_signal(&users_file_cond_var);
            pthread_mutex_unlock(&users_file_mutex);

            return 1;

      }else{
            perror("fopen");
            
            users_file_free = 1;
            pthread_cond_signal(&users_file_cond_var);
            pthread_mutex_unlock(&users_file_mutex);

            return 2;
      }

}

unsigned int registration(char* username, char* password){

      pthread_mutex_lock(&users_file_mutex);

      while(users_file_free == 0)
            pthread_cond_wait(&users_file_cond_var, &users_file_mutex);
      
      users_file_free = 0;

      FILE* file_users = fopen("users.dat", "rb+");

      if(file_users != NULL){
            
            User current_user;

            while(fread(&current_user, sizeof(User), 1, file_users) == 1){

                  if( strcmp(username, current_user.username) == 0 ){
                              
                              fclose(file_users);

                              users_file_free = 1;
                              pthread_cond_signal(&users_file_cond_var);
                              pthread_mutex_unlock(&users_file_mutex);

                              return 1;

                        }
                        

            }
            
            strcpy(current_user.username, username);
            unsigned long hashed_password = hash((unsigned char*)password);
            current_user.hashed_password = hashed_password;

            fwrite(&current_user, sizeof(User), 1, file_users);

            fclose(file_users);

            users_file_free = 1;
            pthread_cond_signal(&users_file_cond_var);
            pthread_mutex_unlock(&users_file_mutex);

            return 0;

      }else{
            perror("fopen");

            users_file_free = 1;
            pthread_cond_signal(&users_file_cond_var);
            pthread_mutex_unlock(&users_file_mutex);

            return 2;
      }

}

unsigned long hash(unsigned char *str){

    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
            hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;

}


ssize_t send_all(int socket_for_thread, const void* buf, size_t n){

      size_t sent = 0;
      const char* ptr = (const char*)buf;

      while(sent < n){

            ssize_t w = send(socket_for_thread, ptr + sent, (size_t)(n - sent), 0);

            if (w < 0) { 

                  if (errno == EINTR) 
                        continue; 
            
                  if (errno == EPIPE) {
                        perror("Client morto\n");
                        close(socket_for_thread);
                        pthread_exit(0);
                  }

                  perror("send"); 
                  close(socket_for_thread); 
                  pthread_exit(0); 
            }

            sent += (size_t)w;
      }

      return (ssize_t)sent;
      
}

ssize_t recv_all(int socket_for_thread, void* buf, size_t n, int flags){

      size_t received = 0;
      char* ptr = (char*)buf;

      while(received < n){

            ssize_t r = recv(socket_for_thread, ptr + received, (size_t)(n - received), flags);

            if( r < 0 ){

                  if(errno == EINTR)
                        continue;

                  if(errno == EAGAIN || errno == EWOULDBLOCK) {
                        if (received == 0) {
                              return -1;
                        } else {
                              continue;
                        }
                  }
                  
                  perror("recv"); 
                  close(socket_for_thread); 
                  pthread_exit(0); 

            }

            if( r == 0 )       //connection closed
                  return (ssize_t)received;


            received += (size_t)r;


      }

      return (ssize_t)received;

}

ssize_t recv_string(int socket_for_thread, char* buf, size_t max_len, int flags) {

      size_t received = 0;
      char c;

      //Read until the max is reached, leaving 1 place for string terminator '\0'
      while (received < max_len - 1) {
            
            ssize_t r = recv(socket_for_thread, &c, (size_t)1, flags); // Reads 1 char

            if (r < 0) {

                  if (errno == EINTR) 
                        continue; 
                  
                  perror("recv");
                  close(socket_for_thread);
                  pthread_exit(0);
            }

            if( r == 0 )       //connection closed
                  return (ssize_t)received;

            // If the char is \n or string terminator \0 stop
            if (c == '\n' || c == '\0') {
            
                  // Check if arrived \r\n instead of only \n
                  if (received > 0 && buf[received - 1] == '\r') {
                        received--;       //Eliminate \r
                  }
                  
                  break;
            }

            buf[received] = c;
            received++;
      }

      buf[received] = '\0';

      return (ssize_t)received;
}


ssize_t send_all_in_match(int socket_for_thread, const void* buf, size_t n, Match_list_node* match_node, User* current_user, int id_in_match){

      size_t sent = 0;
      const void* ptr = buf;

      while(sent < n){

            ssize_t w = send(socket_for_thread, ptr + sent, (size_t)(n - sent), 0);

            if (w < 0) { 

                  if (errno == EINTR) 
                        continue; 
            
                  if (errno == EPIPE) {
                        perror("Client morto\n");
                        handle_client_death_in_lobby(socket_for_thread, match_node, current_user, id_in_match);

                  }

                  perror("send"); 
                  handle_client_death_in_lobby(socket_for_thread, match_node, current_user, id_in_match);

            }

            sent += (size_t)w;
      }

      return (ssize_t)sent;


}

ssize_t recv_all_in_match(int socket_for_thread, void* buf, size_t n, int flags, Match_list_node* match_node, User* current_user, int id_in_match){

      size_t received = 0;
      void* ptr = buf;

      while(received < n){

            ssize_t r = recv(socket_for_thread, ptr + received, (size_t)(n - received), flags);

            if( r < 0 ){

                  if(errno == EINTR)
                        continue;

                  if(errno == EAGAIN || errno == EWOULDBLOCK) {
                        if (received == 0) {
                              return -1;
                        } else {
                              continue;
                        }
                  }
                  
                  perror("recv"); 
                  handle_client_death_in_lobby(socket_for_thread, match_node, current_user, id_in_match); 

            }

            if( r == 0 )       //Connection closed
                  return (ssize_t)received;


            received += (size_t)r;


      }

      return (ssize_t)received;


}



User* handle_starting_interaction(int socket_for_thread){
      
      char start_message[] = "\n        DUNGEONS & COLORS        \n\nBenvenuti!\nInserire il numero relativo all'opzione che si desidera:\n1)Login\n2)Registrazione\n3)Esci\n";

      send_all(socket_for_thread, start_message, sizeof(start_message));

      uint32_t option_received, option;
      int done = 0;

      while (done == 0){

            ssize_t r = recv_all(socket_for_thread, &option_received, sizeof(option), 0);

            if (r == sizeof(option_received)) 
                  option = ntohl(option_received);


            switch (option){
                  case 1:
                        done = 1;
                        return handle_login(socket_for_thread);
                  case 2:
                        done = 1;
                        return handle_registration(socket_for_thread);
                  case 3:
                        done = 1;
                        char closing_message[] = "\nArrivederci!\n";
                        send_all(socket_for_thread, closing_message, sizeof(closing_message));

                        close(socket_for_thread);

                        pthread_exit(0);

                  default:
                        char error_message[] = "\n        DUNGEONS & COLORS        \n\nOpzione non valida!\nInserire il numero relativo all'opzione che si desidera:\n1)Login\n2)Registrazione\n3)Esci\n";
                        send_all(socket_for_thread, error_message, sizeof(error_message));
                        break;
            }

      }
      
      return NULL;

}

User* handle_login(int socket_for_thread){

      char username_request[] = "\nInserire il proprio username: ";
      char password_request[] = "\nInserire la propria password: ";
      char username[64], password[64];

      send_all(socket_for_thread, username_request, sizeof(username_request));
      
      recv_string(socket_for_thread, username, sizeof(username), 0);

      send_all(socket_for_thread, password_request, sizeof(password_request));

      recv_string(socket_for_thread, password, sizeof(password), 0);

      unsigned int res = login(username, password);
      uint32_t status_code;         //status code to send to the client together with the message

      if(res == 0){
            
            User* current_user = malloc(sizeof(User));

            if (!current_user) {

                  perror("malloc");
                  close(socket_for_thread);

                  pthread_exit(0);

            }

            strcpy(current_user->username, username);

            status_code = htonl(0);
            send_all(socket_for_thread, &status_code, sizeof(status_code));

            char login_success_message[] = "\nLogin effettuato con successo!\n";
            send_all(socket_for_thread, login_success_message, sizeof(login_success_message));

            return current_user;

      }else{

            status_code = htonl(1);
            send_all(socket_for_thread, &status_code, sizeof(status_code));

            char login_error_message[] = "\nLogin fallito!\nCosa si desidera fare?\n1)Riprova\n2)Effettuare la registrazione\n3)Esci\n";

            send_all(socket_for_thread, login_error_message, sizeof(login_error_message));

            uint32_t option_received, option;
            int done = 0;

            while (done == 0){

                  ssize_t r = recv_all(socket_for_thread, &option_received, sizeof(option), 0);

                  if (r == sizeof(option_received)) 
                        option = ntohl(option_received);


                  switch (option){
                        case 1:
                              done = 1;
                              return handle_login(socket_for_thread);
                        case 2:
                              done = 1;
                              return handle_registration(socket_for_thread);
                        case 3:
                              done = 1;
                              char closing_message[] = "\nArrivederci!\n";
                              send_all(socket_for_thread, closing_message, sizeof(closing_message));

                              close(socket_for_thread);

                              pthread_exit(0);

                        default:
                              char error_message[] = "\nOpzione non valida!\nInserire il numero relativo all'opzione che si desidera:1)Riprova login\n2)Effettuare la registrazione\n3)Esci\n";
                              send_all(socket_for_thread, error_message, sizeof(error_message));
                              break;
                  }

            }
            


      }
      return NULL;
}

User* handle_registration(int socket_for_thread){

      char username_request[] = "\nInserire il proprio username: ";
      char password_request[] = "\nInserire la propria password: ";
      char username[64], password[64];

      send_all(socket_for_thread, username_request, sizeof(username_request));

      recv_string(socket_for_thread, username, sizeof(username), 0);

      send_all(socket_for_thread, password_request, sizeof(password_request));

      recv_string(socket_for_thread, password, sizeof(password), 0);

      unsigned int res = registration(username, password);
      uint32_t status_code;         //status code to send to the client together with the message

      if(res == 0){
            
            User* current_user = malloc(sizeof(User));

            if (!current_user) {

                  perror("malloc");
                  close(socket_for_thread);

                  pthread_exit(0);

            }

            strcpy(current_user->username, username);

            status_code = htonl(0);
            send_all(socket_for_thread, &status_code, sizeof(status_code));

            char registration_success_message[] = "\nRegistrazione effettuata con successo!\n";
            send_all(socket_for_thread, registration_success_message, sizeof(registration_success_message));

            return current_user;

      }else {

            status_code = htonl(1);
            send_all(socket_for_thread, &status_code, sizeof(status_code));

            char registration_username_error_message[] = "\nRegistrazione fallita perchè lo username è gia' in uso!\nCosa si desidera fare?\n1)Riprova\n2)Effettuare il login\n3)Esci\n";
            char registration_generic_error_message[] = "\nRegistrazione fallita!\nCosa si desidera fare?\n1)Riprova\n2)Effettuare il login\n3)Esci\n";
            
            if(res == 1)
                  send_all(socket_for_thread, registration_username_error_message, sizeof(registration_username_error_message));
            else
                  send_all(socket_for_thread, registration_generic_error_message, sizeof(registration_generic_error_message));
            
            uint32_t option_received, option;
            int done = 0;

            while (done == 0){
                  
                  ssize_t r = recv_all(socket_for_thread, &option_received, sizeof(option), 0);

                  if (r == sizeof(option_received)) 
                        option = ntohl(option_received);


                  switch (option){
                        case 1:
                              done = 1;
                              return handle_registration(socket_for_thread);
                        case 2:
                              done = 1;
                              return handle_login(socket_for_thread);
                        case 3:
                              done = 1;
                              char closing_message[] = "\nArrivederci!\n";
                              send_all(socket_for_thread, closing_message, sizeof(closing_message));

                              close(socket_for_thread);

                              pthread_exit(0);

                        default:
                              char error_message[] = "\nOpzione non valida!\nInserire il numero relativo all'opzione che si desidera:\n1)Riprova\n2)Effettuare il login\n3)Esci\n";
                              send_all(socket_for_thread, error_message, sizeof(error_message));
                              break;
                  }

            }
            


      }
      return NULL;
}


void handle_session(int socket_for_thread, User* current_user){

      char welcome_message_and_menu[] = "\n        DUNGEONS & COLORS        \n\nBenvenuti nel gioco!\nInserire il numero relativo all'opzione che si desidera:\n1)Crea lobby\n2)Entra in una lobby\n3)Esci\n";

      send_all(socket_for_thread, welcome_message_and_menu, sizeof(welcome_message_and_menu));

      uint32_t option_received, option;
      int done = 0;

      while (done == 0){

            ssize_t r = recv_all(socket_for_thread, &option_received, sizeof(option), 0);

            if (r == sizeof(option_received)) 
                  option = ntohl(option_received);


            switch (option){
                  case 1:
                        done = 1;
                        lobby_creation(socket_for_thread, current_user);
                        break;
                  case 2:
                        done = 1;
                        join_lobby(socket_for_thread, current_user);
                        break;
                  case 3:
                        done = 1;
                        char closing_message[] = "\nArrivederci!\n";
                        send_all(socket_for_thread, closing_message, sizeof(closing_message));
                        
                        free(current_user);
                        close(socket_for_thread);

                        pthread_exit(0);

                  default:
                        char error_message[] = "\n        DUNGEONS & COLORS        \n\nOpzione non valida!\nInserire il numero relativo all'opzione che si desidera:\n1)Crea lobby\n2)Entra in una lobby\n3)Esci\n";
                        send_all(socket_for_thread, error_message, sizeof(error_message));
                        break;
            }

      }

}

void lobby_creation(int socket_for_thread, User* current_user){

      char size_request[] = "\nInserire l'opzione relativa alla dimensione della mappa con cui si desidera giocare:\n1)16x16\n2)32x32\n3)48x48\n";

      send_all(socket_for_thread, size_request, sizeof(size_request));

      uint32_t size;
      uint32_t option_received, option;
      int done = 0;

      while (done == 0){

            ssize_t r = recv_all(socket_for_thread, &option_received, sizeof(option), 0);

            if (r == sizeof(option_received)) 
                  option = ntohl(option_received);


            switch (option){
                  case 1:
                        done = 1;
                        size = 16;
                        break;
                  case 2:
                        done = 1;
                        size = 32;
                        break;
                  case 3:
                        done = 1;
                        size = 48;
                        break;

                  default:
                        char error_message[] = "\nOpzione non valida!\nInserire il numero relativo alla dimensione della mappa che si desidera:\n1)16x16\n2)32x32\n3)48x48\n";
                        send_all(socket_for_thread, error_message, sizeof(error_message));
                        break;
            }

      }

      Match* new_match = init_match(socket_for_thread, current_user, size);
      
      Match_list_node* new_node = malloc(sizeof(Match_list_node));
      if (!new_node) {

            perror("malloc");
            close(socket_for_thread);

            pthread_exit(0);
      }

      if(match_list == NULL){
            new_node->match = new_match;
            new_node->next = NULL;
            match_list = new_node;
      }else{
            new_node->match = new_match;
            new_node->next = match_list;
            match_list = new_node;
      }

      handle_being_in_lobby(socket_for_thread, new_node, current_user, 0);      //0 as id_in_match because the lobby has been just created by this user, so he is the only user and also the host

}

void join_lobby(int socket_for_thread, User* current_user){

      char* message_with_matches_info = get_message_with_matches_info(socket_for_thread, 0);     //normal call, no error occurred

      uint32_t status_code;         //status code to send to the client together with the message

      send_all(socket_for_thread, message_with_matches_info, strlen(message_with_matches_info) + 1);

      if(strcmp(message_with_matches_info, "\nNon sono disponili lobby in cui entrare!\n") == 0){
            free(message_with_matches_info);
            message_with_matches_info = NULL;

            status_code = htonl(1);       //If no lobbies are available, send error status code
            send_all(socket_for_thread, &status_code, sizeof(status_code));

            handle_session(socket_for_thread, current_user);
      }

      status_code = htonl(0);       //Otherwise, send ok status code
      send_all(socket_for_thread, &status_code, sizeof(status_code));
            
      uint32_t option_received, option;
      int done = 0;

      while (done == 0){

            ssize_t r = recv_all(socket_for_thread, &option_received, sizeof(option), 0);

            if (r == sizeof(option_received)) 
                  option = ntohl(option_received);



            if(option == 0){
                  handle_session(socket_for_thread, current_user);
                  return;
            }

            Match_list_node* tmp = match_list;

            while(tmp != NULL){

                  if( option == tmp->match->id  &&  tmp->match->status == NOT_IN_PROGRESS ){

                        free(message_with_matches_info);
                        message_with_matches_info = NULL;
                        int res = join_spcific_lobby(socket_for_thread, current_user, tmp);
                        
                        if(res == 0){
                              done = 1;
                              return;
                        }else
                              break;
                  }

                  tmp = tmp->next;

            }

            status_code = htonl(1);       //Otherwise, send error status code
            send_all(socket_for_thread, &status_code, sizeof(status_code));

            free(message_with_matches_info);
            message_with_matches_info = get_message_with_matches_info(socket_for_thread, 1);     //Id specified not available, call with error_flag set
            send_all(socket_for_thread, message_with_matches_info, strlen(message_with_matches_info) + 1);

            if(strcmp(message_with_matches_info, "\nNon sono disponili lobby in cui entrare!\n") == 0){

                  status_code = htonl(1);       //If no lobbies are available, send error status code
                  send_all(socket_for_thread, &status_code, sizeof(status_code));

                  free(message_with_matches_info);
                  message_with_matches_info = NULL;
                  handle_session(socket_for_thread, current_user);
                  return;
            }

            status_code = htonl(0);       //Otherwise, send ok status code
            send_all(socket_for_thread, &status_code, sizeof(status_code));

      }


      
}

char* get_message_with_matches_info(int socket_for_thread, int error_flag){

      char* matches_info = malloc(sizeof(char) * 4096);

      if (!matches_info) {

            perror("malloc");
            close(socket_for_thread);

            pthread_exit(0);
      }
      
      if(error_flag == 0)
            strcpy(matches_info, "\nInserire l'id della lobby in cui si desidera entrare oppure 0 per annullare:\nLobby disponibili (Id\tDimensione mappa di gioco\tGiocatori connessi):\n");
      else
            strcpy(matches_info, "Id lobby specificato non disponibile!\nInserire l'id della lobby in cui si desidera entrare oppure 0 per annullare:\nLobby disponibili (Id\tDimensione mappa di gioco\tGiocatori connessi):\n");


      int lobby_found = 0;          //Checks if at least a lobby is available

      Match_list_node* tmp = match_list;

      int i;

      while(tmp != NULL){

            pthread_mutex_lock(&tmp->match->match_mutex);

            while(tmp->match->match_free == 0)
                  pthread_cond_wait(&tmp->match->match_cond_var, &tmp->match->match_mutex);

            tmp->match->match_free = 0;


            if(tmp->match->status == NOT_IN_PROGRESS){       //It's not possible to enter in a match already in progress, so that lobby is not available

                  int full = 1;
                  for(i = 0; i < MAX_PLAYERS_MATCH; i++)
                        if(tmp->match->players[i] == NULL){       
                              full = 0;
                              break;
                        }

                  
                  if(full == 0){                            //It's not possible to enter a lobby full, so that lobby is not available

                        lobby_found = 1;

                        char string_number[10] = {0};

                        sprintf(string_number, "%u", tmp->match->id);

                        strcat(matches_info, string_number);

                        strcat(matches_info, "\t");

                        sprintf(string_number, "%d", tmp->match->size);
                        strcat(string_number, "x");
                        sprintf(string_number, "%d", tmp->match->size);

                        strcat(matches_info, string_number);

                        for(i=0; i < MAX_PLAYERS_MATCH; i++){

                              if(tmp->match->players[i] != NULL){

                                    
                                    strcat(matches_info, "\t");
                                    
                                    strcat(matches_info, tmp->match->players[i]->username);

                              }
                                    
                        }

                        strcat(matches_info, "\n");
                  }

                  

            }

            tmp->match->match_free = 1;
            pthread_cond_signal(&tmp->match->match_cond_var);
            pthread_mutex_unlock(&tmp->match->match_mutex);
            
            tmp = tmp->next;

      }

      if(lobby_found == 0){         //If no available lobbies were found, send a message to say that
            
            strcpy(matches_info, "\nNon sono disponili lobby in cui entrare!\n");

      }

      return matches_info;

}

unsigned int join_spcific_lobby(int socket_for_thread, User* current_user, Match_list_node* match_to_join){

      int i;

      pthread_mutex_lock(&match_to_join->match->match_mutex);

      while(match_to_join->match->match_free == 0)
            pthread_cond_wait(&match_to_join->match->match_cond_var, &match_to_join->match->match_mutex);

      match_to_join->match->match_free = 0;


      for(i = 0; i < MAX_PLAYERS_MATCH; i++){
            if(match_to_join->match->players[i] == NULL){
                  
                  User_in_match* new_player = malloc(sizeof(User_in_match));
                  strcpy(new_player->username, current_user->username);
                  new_player->tid = pthread_self();
                  match_to_join->match->players[i] = new_player;

                  match_to_join->match->match_free = 1;
                  pthread_cond_signal(&match_to_join->match->match_cond_var);
                  pthread_mutex_unlock(&match_to_join->match->match_mutex);

                  uint32_t status_code = htonl(0);       //If joined the specific lobby, send ok status code
                  send_all(socket_for_thread, &status_code, sizeof(status_code));
                  
                  handle_being_in_lobby(socket_for_thread, match_to_join, current_user, i);
                  return 0;         //Founded a place for the user, all ok

            }
      }

      

      match_to_join->match->match_free = 1;
      pthread_cond_signal(&match_to_join->match->match_cond_var);
      pthread_mutex_unlock(&match_to_join->match->match_mutex);

      return 1;         //Lobby full, error returned


}

void handle_being_in_lobby(int socket_for_thread, Match_list_node* match_node, User* current_user, int id_in_match){

      char message_host[1024] = "\n        DUNGEONS & COLORS        \n\nSei nella tua lobby!\nInserire il numero relativo all'opzione che si desidera:\n1)Inizia partita\n2)Cambia dimensione mappa\n3)Esci dalla lobby\n4)Esci dal gioco\n";
      char message_guest[1024] = "\n        DUNGEONS & COLORS        \n\nSei in una lobby!\nAttendere l'inizio della partita oppure inserire il numero relativo all'opzione che si desidera:\n1)Esci dalla lobby\n2)Esci dal gioco\n";

      uint32_t code = 1;         //Code to send to the client together with the message; 0 = role host, 1 = role guest, 2 = match started

      char* list_of_players_and_size = get_message_list_of_players_and_size(socket_for_thread, current_user, match_node, id_in_match);

      pthread_mutex_lock(&match_node->match->match_mutex);

      while(match_node->match->match_free == 0)
            pthread_cond_wait(&match_node->match->match_cond_var, &match_node->match->match_mutex);

      match_node->match->match_free = 0;

      if(id_in_match == match_node->match->host){

            match_node->match->match_free = 1;
            pthread_cond_signal(&match_node->match->match_cond_var);
            pthread_mutex_unlock(&match_node->match->match_mutex);

            code = htonl(0);
            send_all_in_match(socket_for_thread, &code, sizeof(code), match_node, current_user, id_in_match);

            strcat(message_host, list_of_players_and_size);
            send_all_in_match(socket_for_thread, message_host, strlen(message_host) + 1, match_node, current_user, id_in_match);

      }else{
            match_node->match->match_free = 1;
            pthread_cond_signal(&match_node->match->match_cond_var);
            pthread_mutex_unlock(&match_node->match->match_mutex);

            code = htonl(1);
            send_all_in_match(socket_for_thread, &code, sizeof(code), match_node, current_user, id_in_match);

            strcat(message_guest, list_of_players_and_size);
            send_all_in_match(socket_for_thread, message_guest, strlen(message_guest) + 1, match_node, current_user, id_in_match);

      }

      
      time_t last = time(NULL);

      for (;;) {

            uint32_t option_received, option;

            ssize_t r = recv_all_in_match(socket_for_thread, &option_received, sizeof(option), MSG_DONTWAIT, match_node, current_user, id_in_match);

            if (r > 0) {

                  if (r == sizeof(option_received)) 
                        option = ntohl(option_received);
                  

                  pthread_mutex_lock(&match_node->match->match_mutex);

                  while(match_node->match->match_free == 0)
                        pthread_cond_wait(&match_node->match->match_cond_var, &match_node->match->match_mutex);

                  match_node->match->match_free = 0;

                  if(id_in_match == match_node->match->host){

                        match_node->match->match_free = 1;
                        pthread_cond_signal(&match_node->match->match_cond_var);
                        pthread_mutex_unlock(&match_node->match->match_mutex);


                        switch (option){
                              case 1:
                                    start_match(socket_for_thread, match_node, current_user, id_in_match);
                                    //after match ending, back in this function

                                    if(list_of_players_and_size != NULL) 
                                          free(list_of_players_and_size);

                                    list_of_players_and_size = get_message_list_of_players_and_size(socket_for_thread, current_user, match_node, id_in_match);
                                    
                                    strcpy(message_host, "\n        DUNGEONS & COLORS        \n\nSei nella tua lobby!\nInserire il numero relativo all'opzione che si desidera:\n1)Inizia partita\n2)Cambia dimensione mappa\n3)Esci dalla lobby\n4)Esci dal gioco\n");
                                    
                                    strcat(message_host, list_of_players_and_size);
                                    
                                    code = htonl(0);
                                    send_all_in_match(socket_for_thread, &code, sizeof(code), match_node, current_user, id_in_match);

                                    send_all_in_match(socket_for_thread, message_host, strlen(message_host) + 1, match_node, current_user, id_in_match);
                                    
                                    break;

                              case 2:
                                    change_map_size(socket_for_thread, match_node, current_user, id_in_match);

                                    if(list_of_players_and_size != NULL) 
                                          free(list_of_players_and_size);

                                    list_of_players_and_size = get_message_list_of_players_and_size(socket_for_thread, current_user, match_node, id_in_match);
                                    
                                    strcpy(message_host, "\n        DUNGEONS & COLORS        \n\nSei nella tua lobby!\nInserire il numero relativo all'opzione che si desidera:\n1)Inizia partita\n2)Cambia dimensione mappa\n3)Esci dalla lobby\n4)Esci dal gioco\n");
                                    
                                    strcat(message_host, list_of_players_and_size);

                                    code = htonl(0);
                                    send_all_in_match(socket_for_thread, &code, sizeof(code), match_node, current_user, id_in_match);

                                    send_all_in_match(socket_for_thread, message_host, strlen(message_host) + 1, match_node, current_user, id_in_match);
                                    break;
                              case 3:
                                    handle_leaving_lobby(match_node, id_in_match);
                                    handle_session(socket_for_thread, current_user);
                                    return;
                              case 4:
                                    handle_client_death_in_lobby(socket_for_thread, match_node, current_user, id_in_match);
                                    return;

                              default:
                                    char error_message[] = "\n        DUNGEONS & COLORS        \n\nOpzione non valida!\nInserire il numero relativo all'opzione che si desidera:\n1)Inizia partita\n2)Cambia dimensione mappa\n3)Esci dalla lobby\n4)Esci dal gioco\n";
                                    
                                    if(list_of_players_and_size != NULL)
                                          free(list_of_players_and_size);

                                    list_of_players_and_size = get_message_list_of_players_and_size(socket_for_thread, current_user, match_node, id_in_match);
                                    strcat(error_message, list_of_players_and_size);
                                    
                                    code = htonl(0);
                                    send_all_in_match(socket_for_thread, &code, sizeof(code), match_node, current_user, id_in_match);

                                    send_all_in_match(socket_for_thread, error_message, strlen(error_message) + 1, match_node, current_user, id_in_match);
                                    break;
                        }

                  }else{

                        match_node->match->match_free = 1;
                        pthread_cond_signal(&match_node->match->match_cond_var);
                        pthread_mutex_unlock(&match_node->match->match_mutex);


                        switch (option){
                              case 1:
                                    handle_leaving_lobby(match_node, id_in_match);
                                    handle_session(socket_for_thread, current_user);
                                    return;
                              case 2:
                                    handle_client_death_in_lobby(socket_for_thread, match_node, current_user, id_in_match);
                                    return;

                              default:
                                    char error_message[] = "\n        DUNGEONS & COLORS        \n\nOpzione non valida!\nInserire il numero relativo alla dimensione della mappa che si desidera:\n1)Esci dalla lobby\n2)Esci dal gioco\n";

                                    if(list_of_players_and_size != NULL)
                                          free(list_of_players_and_size);
                                    
                                    list_of_players_and_size = get_message_list_of_players_and_size(socket_for_thread, current_user, match_node, id_in_match);
                                    strcat(error_message, list_of_players_and_size);
                                    
                                    code = htonl(1);
                                    send_all_in_match(socket_for_thread, &code, sizeof(code), match_node, current_user, id_in_match);

                                    send_all_in_match(socket_for_thread, error_message, strlen(error_message) + 1, match_node, current_user, id_in_match);
                                    break;
                        }


                  }
            } else if (r == 0) {
                  // client closed
                  handle_client_death_in_lobby(socket_for_thread, match_node, current_user, id_in_match);
                  break;

            } else {
                  // r < 0
                  if (errno != EAGAIN && errno != EWOULDBLOCK) {
                        perror("recv");
                        break;
                  }
                  //no data, all normal
            }

            //Checks if the match started, in which case it enters the match
            pthread_mutex_lock(&match_node->match->match_mutex);

            while(match_node->match->match_free == 0)
                  pthread_cond_wait(&match_node->match->match_cond_var, &match_node->match->match_mutex);

            match_node->match->match_free = 0;

            if(id_in_match != match_node->match->host && match_node->match->status == IN_PROGRESS){

                  match_node->match->match_free = 1;
                  pthread_cond_signal(&match_node->match->match_cond_var);
                  pthread_mutex_unlock(&match_node->match->match_mutex);
                  
                  code = htonl(2);
                  send_all_in_match(socket_for_thread, &code, sizeof(code), match_node, current_user, id_in_match);

                  handle_being_in_match(socket_for_thread, match_node, current_user, id_in_match);
                  //no return, after match ending, back in this function
                  if(list_of_players_and_size != NULL) 
                        free(list_of_players_and_size);

                  list_of_players_and_size = get_message_list_of_players_and_size(socket_for_thread, current_user, match_node, id_in_match);
                  
                  strcpy(message_guest, "\n        DUNGEONS & COLORS        \n\nSei in una lobby!\nAttendere l'inizio della partita oppure inserire il numero relativo all'opzione che si desidera:\n1)Esci dalla lobby\n2)Esci dal gioco\n");
                  
                  strcat(message_guest, list_of_players_and_size);
                  
                  code = htonl(1);
                  send_all_in_match(socket_for_thread, &code, sizeof(code), match_node, current_user, id_in_match);

                  send_all_in_match(socket_for_thread, message_guest, strlen(message_guest) + 1, match_node, current_user, id_in_match);

            }else{
                  
                  match_node->match->match_free = 1;
                  pthread_cond_signal(&match_node->match->match_cond_var);
                  pthread_mutex_unlock(&match_node->match->match_mutex);

            }


            //Every second checks if players or size changed, and if so re-sends the message changing the list of players connected and/or the size
            time_t now = time(NULL);
            if (now - last >= 1) {

                  char* new_list_of_players_and_size = get_message_list_of_players_and_size(socket_for_thread, current_user, match_node, id_in_match);

                  if(strcmp(list_of_players_and_size, new_list_of_players_and_size) != 0){

                        if(list_of_players_and_size != NULL)
                              free(list_of_players_and_size);
                        
                        list_of_players_and_size = new_list_of_players_and_size;
                        new_list_of_players_and_size = NULL;
                        
                        pthread_mutex_lock(&match_node->match->match_mutex);

                        while(match_node->match->match_free == 0)
                              pthread_cond_wait(&match_node->match->match_cond_var, &match_node->match->match_mutex);

                        match_node->match->match_free = 0;

                        if(id_in_match == match_node->match->host){

                              match_node->match->match_free = 1;
                              pthread_cond_signal(&match_node->match->match_cond_var);
                              pthread_mutex_unlock(&match_node->match->match_mutex);

                              strcpy(message_host, "\n        DUNGEONS & COLORS        \n\nSei nella tua lobby!\nInserire il numero relativo all'opzione che si desidera:\n1)Inizia partita\n2)Cambia dimensione mappa\n3)Esci dalla lobby\n4)Esci dal gioco\n");
                            
                              strcat(message_host, list_of_players_and_size);

                              code = htonl(0);
                              send_all_in_match(socket_for_thread, &code, sizeof(code), match_node, current_user, id_in_match);

                              send_all_in_match(socket_for_thread, message_host, strlen(message_host) + 1, match_node, current_user, id_in_match);

                        }else{
                              match_node->match->match_free = 1;
                              pthread_cond_signal(&match_node->match->match_cond_var);
                              pthread_mutex_unlock(&match_node->match->match_mutex);

                              strcpy(message_guest, "\n        DUNGEONS & COLORS        \n\nSei in una lobby!\nAttendere l'inizio della partita oppure inserire il numero relativo all'opzione che si desidera:\n1)Esci dalla lobby\n2)Esci dal gioco\n");

                              strcat(message_guest, list_of_players_and_size);

                              code = htonl(1);
                              send_all_in_match(socket_for_thread, &code, sizeof(code), match_node, current_user, id_in_match);

                              send_all_in_match(socket_for_thread, message_guest, strlen(message_guest) + 1, match_node, current_user, id_in_match);

                        }


                  }

                  last = now;

            }

            usleep(10000); // 10ms
      }
      
}

char* get_message_list_of_players_and_size(int socket_for_thread, User* current_user, Match_list_node* match_node, int id_in_match){

      if(match_node != NULL){
            if(match_node->match != NULL){

                  int i;
                  char* list_of_players_and_size = malloc(sizeof(char)*(22 + 64*4 + 3 + 30 + 1));    //Introduction + maximum size of 4 usernames + at most 3 \t + size of the map + the string terminator
                  
                  if (!list_of_players_and_size) {

                        perror("malloc");
                        handle_client_death_in_lobby(socket_for_thread, match_node, current_user, id_in_match);
                  
                  }

                  strcpy(list_of_players_and_size, "\nGiocatori connessi: ");

                  pthread_mutex_lock(&match_node->match->match_mutex);

                  while(match_node->match->match_free == 0)
                        pthread_cond_wait(&match_node->match->match_cond_var, &match_node->match->match_mutex);

                  match_node->match->match_free = 0;

                  for(i=0; i < MAX_PLAYERS_MATCH; i++){

                        if(match_node->match->players[i] != NULL){

                              if (i > 0) {
                                    strcat(list_of_players_and_size, "\t");
                              }

                              strcat(list_of_players_and_size, match_node->match->players[i]->username);

                        }
                              
                  }

                  strcat(list_of_players_and_size, "\nDimensione mappa: ");

                  char string_size[5] = {0}, string_dimension[15] = {0};
                  sprintf(string_size, "%d", match_node->match->size);

                  strcat(string_dimension, string_size);
                  strcat(string_dimension, "x");
                  strcat(string_dimension, string_size);
                  strcat(string_dimension, "\n");

                  strcat(list_of_players_and_size, string_dimension);
                  
                  match_node->match->match_free = 1;
                  pthread_cond_signal(&match_node->match->match_cond_var);
                  pthread_mutex_unlock(&match_node->match->match_mutex);

                  return list_of_players_and_size;
            }
      }

      handle_client_death_in_lobby(socket_for_thread, match_node, current_user, id_in_match);
      return NULL;
}

void start_match(int socket_for_thread, Match_list_node* match_node, User* current_user, int id_in_match){

      pthread_mutex_lock(&match_node->match->match_mutex);

      while(match_node->match->match_free == 0)
            pthread_cond_wait(&match_node->match->match_cond_var, &match_node->match->match_mutex);

      match_node->match->match_free = 0;

      int i;
      //Random creation of the map
      int j, x, y, z, k;
      int num_of_blocks = match_node->match->size/16;        //Number of 16x16 blocks of the map
      for(i=0; i < num_of_blocks; i++){

            for(j=0; j < num_of_blocks; j++){
                  
                  int random_preset = rand() % 4;
                  z = 0;
                  

                  for(x = i*16; x < (i+1)*16; x++, z++){

                        k = 0;

                        for(y = j*16; y < (j+1)*16; y++, k++){

                              match_node->match->map[x][y] = array_of_map_presets[random_preset][z][k];

                        }


                  }


            }


      }

      uint32_t size = match_node->match->size;

      //Inizialize player position to impossible values (to avoid garbage values)
      for(i=0; i < MAX_PLAYERS_MATCH; i++){
            if(match_node->match->players[i] != NULL){
                  match_node->match->players[i]->x = size + 1;
                  match_node->match->players[i]->y = size + 1;
            }
      }

      int pos_ok = 0;
      //Random positioning the players
      for(i=0; i < MAX_PLAYERS_MATCH; i++){

            pos_ok=0;
            if(match_node->match->players[i] != NULL){

                  while(pos_ok == 0){

                        pos_ok = 1;       //tries to put pos_ok
                        x = rand() % size;
                        y = rand() % size;

                        match_node->match->players[i]->x = x;
                        match_node->match->players[i]->y = y;

                        if(match_node->match->map[x][y] == 'e'){
                              for(j=0; j < MAX_PLAYERS_MATCH; j++){
                                    if( j != i && match_node->match->players[j] != NULL)
                                          if(match_node->match->players[j]->x == x && match_node->match->players[j]->y == y){
                                                pos_ok = 0;
                                                break;
                                          }

                              }
                        }else{
                              pos_ok = 0;
                        }


                  }

                  match_node->match->map[x][y] = colors[i];

            }


      }

      match_node->match->status = IN_PROGRESS;

      /*for(i=0; i < MAX_PLAYERS_MATCH; i++){
            if(i != id_in_match && match_node->match->players[i] != NULL)
                  pthread_kill(match_node->match->players[i]->tid, SIGUSR1);
      }*/

      match_node->match->match_free = 1;
      pthread_cond_signal(&match_node->match->match_cond_var);
      pthread_mutex_unlock(&match_node->match->match_mutex);

      handle_being_in_match(socket_for_thread, match_node, current_user, id_in_match);



}

void change_map_size(int socket_for_thread, Match_list_node* match_node, User* current_user, int id_in_match){

      char size_request[] = "\nInserire l'opzione relativa alla dimensione della mappa con cui si desidera giocare:\n1)16x16\n2)32x32\n3)48x48\n";

      send_all(socket_for_thread, size_request, sizeof(size_request));

      uint32_t size;
      uint32_t option_received, option;
      int done = 0;

      while (done == 0){

            ssize_t r = recv_all(socket_for_thread, &option_received, sizeof(option), 0);

            if (r == sizeof(option_received)) 
                  option = ntohl(option_received);


            switch (option){
                  case 1:
                        done = 1;
                        size = 16;
                        break;
                  case 2:
                        done = 1;
                        size = 32;
                        break;
                  case 3:
                        done = 1;
                        size = 48;
                        break;

                  default:
                        char error_message[] = "\nOpzione non valida!\nInserire il numero relativo alla dimensione della mappa che si desidera:\n1)16x16\n2)32x32\n3)48x48\n";
                        send_all(socket_for_thread, error_message, sizeof(error_message));
                        break;
            }

      }

      if(size != match_node->match->size){

            int i;        

            pthread_mutex_lock(&match_node->match->match_mutex);

            while(match_node->match->match_free == 0)
                  pthread_cond_wait(&match_node->match->match_cond_var, &match_node->match->match_mutex);

            match_node->match->match_free = 0;

            //Deallocating previous map
            for(i = 0; i < match_node->match->size; i++)
                  free(match_node->match->map[i]);
            
            free(match_node->match->map);

            match_node->match->size = size;

            //Allocating the new map
            match_node->match->map = malloc(size*sizeof(char*));

            if (!(match_node->match->map)) {

                  perror("malloc");
                  close(socket_for_thread);

                  pthread_exit(0);
            }

            for(i=0; i < size; i++){

                  match_node->match->map[i] = malloc(size*sizeof(char));
                  if (!(match_node->match->map[i])) {

                        perror("malloc");
                        close(socket_for_thread);

                        pthread_exit(0);
                  }

            }

            match_node->match->match_free = 1;
            pthread_cond_signal(&match_node->match->match_cond_var);
            pthread_mutex_unlock(&match_node->match->match_mutex);
            
      }
      

}

void handle_leaving_lobby(Match_list_node* match_node, int id_in_match){

      pthread_mutex_lock(&match_node->match->match_mutex);

      while(match_node->match->match_free == 0)
            pthread_cond_wait(&match_node->match->match_cond_var, &match_node->match->match_mutex);

      match_node->match->match_free = 0;

      if(id_in_match < MAX_PLAYERS_MATCH){
            free(match_node->match->players[id_in_match]);
            match_node->match->players[id_in_match] = NULL;
      }
            


      match_node->match->match_free = 1;
      pthread_cond_signal(&match_node->match->match_cond_var);
      pthread_mutex_unlock(&match_node->match->match_mutex);

      check_host_change_or_match_cancellation(match_node, id_in_match);

}

void check_host_change_or_match_cancellation(Match_list_node* match_node, int id_in_match){

      int i;
      int found_another_host = 0;

      pthread_mutex_lock(&match_node->match->match_mutex);

      while(match_node->match->match_free == 0)
            pthread_cond_wait(&match_node->match->match_cond_var, &match_node->match->match_mutex);

      match_node->match->match_free = 0;

      if(id_in_match == match_node->match->host){

            for(i=0; i < MAX_PLAYERS_MATCH; i++)
                  if(match_node->match->players[i] != NULL){

                        found_another_host = 1;
                        match_node->match->host = i;

                  }


            match_node->match->match_free = 1;
            pthread_cond_signal(&match_node->match->match_cond_var);
            pthread_mutex_unlock(&match_node->match->match_mutex);

            if(found_another_host == 0){        //It means that there are no more players in the lobby, so the lobby must be deleted

                  Match_list_node* tmp = match_list;

                  if(tmp == match_node){
                        match_list = match_list->next;
                  }else{
                        while(tmp != NULL){
                              if(tmp->next == match_node){
                                    tmp->next = match_node->next;
                                    break;
                              }
                              tmp = tmp->next;
                        }
                  }

                  free_match(match_node->match);
                  free(match_node);
                  match_node = NULL;
            }

      }else{

            match_node->match->match_free = 1;
            pthread_cond_signal(&match_node->match->match_cond_var);
            pthread_mutex_unlock(&match_node->match->match_mutex);

      }

      


}

void handle_client_death_in_lobby(int socket_for_thread, Match_list_node* match_node, User* current_user, int id_in_match){

      int i;
      int found_another_host = 0;

      pthread_mutex_lock(&match_node->match->match_mutex);

      while(match_node->match->match_free == 0)
            pthread_cond_wait(&match_node->match->match_cond_var, &match_node->match->match_mutex);

      match_node->match->match_free = 0;

      if(id_in_match < MAX_PLAYERS_MATCH){
            free(match_node->match->players[id_in_match]);
            match_node->match->players[id_in_match] = NULL;
      }

      if(id_in_match == match_node->match->host){

            for(i=0; i < MAX_PLAYERS_MATCH; i++)
                  if(match_node->match->players[i] != NULL){

                        found_another_host = 1;
                        match_node->match->host = i;

                  }

            match_node->match->match_free = 1;
            pthread_cond_signal(&match_node->match->match_cond_var);
            pthread_mutex_unlock(&match_node->match->match_mutex);

            if(found_another_host == 0){        //It means that there are no more players in the lobby, so the lobby must be deleted

                  Match_list_node* tmp = match_list;

                  if(tmp == match_node){
                        match_list = match_list->next;
                  }else{
                        while(tmp != NULL){
                              if(tmp->next == match_node){
                                    tmp->next = match_node->next;
                                    break;
                              }
                              tmp = tmp->next;
                        }
                  }

                  free_match(match_node->match);
                  free(match_node);
                  match_node = NULL;
            }

      }else{

            match_node->match->match_free = 1;
            pthread_cond_signal(&match_node->match->match_cond_var);
            pthread_mutex_unlock(&match_node->match->match_mutex);

      }

      
      close(socket_for_thread);
      pthread_exit(0);

}


void handle_being_in_match(int socket_for_thread, Match_list_node* match_node, User* current_user, int id_in_match){

      uint32_t size = match_node->match->size;
      int timer = (size / 16) * SECONDS_PER_SIZE;

      uint32_t message_type = 0;          //Code to send to the client to tell message type, 0 = local information, 1 = global information, 2 = match ending

      //First, sends map's size to the client, to let it allocate its map
      uint32_t size_to_send = htonl(size);
      send_all(socket_for_thread, &size_to_send, sizeof(size_to_send));

      Message_with_local_information* message_with_local_information = get_message_with_local_information(match_node, id_in_match);

      message_type = htonl(0);
      send_all(socket_for_thread, &message_type, sizeof(message_type));

      send_all_in_match(socket_for_thread, message_with_local_information, sizeof(Message_with_local_information), match_node, current_user, id_in_match);

      time_t last_global_map = time(NULL);
      time_t match_start = last_global_map;

      for (;;) {

            char option;

            ssize_t r = recv_all_in_match(socket_for_thread, &option, sizeof(option), MSG_DONTWAIT, match_node, current_user, id_in_match);

            if (r > 0) {

                  handle_move_in_match(match_node, id_in_match, option);
                  free(message_with_local_information);
                  message_with_local_information = get_message_with_local_information(match_node, id_in_match);

                  message_type = htonl(0);
                  send_all(socket_for_thread, &message_type, sizeof(message_type));

                  send_all_in_match(socket_for_thread, message_with_local_information, sizeof(Message_with_local_information), match_node, current_user, id_in_match);


            } else if (r == 0) {
                  // client closed
                  handle_client_death_in_lobby(socket_for_thread, match_node, current_user, id_in_match);
                  break;

            } else {
                  // r < 0
                  if (errno != EAGAIN && errno != EWOULDBLOCK) {
                        perror("recv");
                        break;
                  }
                  //no data, all normal
            }


            //Every TIMER_GLOBAL_MAP sends the global map
            time_t now = time(NULL);
            if (now - last_global_map >= TIMER_GLOBAL_MAP) {

                  message_type = htonl(1);
                  send_all(socket_for_thread, &message_type, sizeof(message_type));

                  send_global_map_to_client(socket_for_thread, match_node, current_user, id_in_match);
                  last_global_map = now;

            }

            //When match timer ends, stops the match
            now = time(NULL);
            if (now - match_start >= timer) {

                  free(message_with_local_information);

                  message_type = htonl(2);
                  send_all(socket_for_thread, &message_type, sizeof(message_type));

                  handle_match_ending(socket_for_thread, match_node, current_user, id_in_match);
                  return;

            }

            usleep(10000); // 10ms
      }

}

Message_with_local_information* get_message_with_local_information(Match_list_node* match_node, int id_in_match){

      const char message[] = "\nMuoversi con w, a, s, d";

      Message_with_local_information* message_with_local_information = malloc(sizeof(Message_with_local_information));

      strcpy(message_with_local_information->message, message);

      uint32_t size = match_node->match->size;

      pthread_mutex_lock(&match_node->match->match_mutex);

      while(match_node->match->match_free == 0)
            pthread_cond_wait(&match_node->match->match_cond_var, &match_node->match->match_mutex);

      match_node->match->match_free = 0;

      int x = match_node->match->players[id_in_match]->x;
      int y = match_node->match->players[id_in_match]->y;

      int i, j, z, k;
      z = 0;
      for(i= x - 2; i <= x + 2; i++, z ++){

            k = 0;
            for(j = y - 2; j <= y + 2; j++, k++ ){

                  if( (i >= 0 && i < size)  && (j >= 0 && j < size) ){            //If i and j are "good" indexes

                        message_with_local_information->local_map[z][k] = match_node->match->map[i][j];

                  }else{

                        message_with_local_information->local_map[z][k] = 'e';

                  }


            }
      }

      for(int i = 0; i < MAX_PLAYERS_MATCH; i++) {

            if(match_node->match->players[i] != NULL) {

                  z = match_node->match->players[i]->x;
                  k = match_node->match->players[i]->y;

                  if( ( z >= (x -2) && z <= (x + 2) ) && ( k >= (y -2) && k <= (y + 2) ) ){
                        
                        message_with_local_information->players_position[i][0] = htonl((uint32_t)z);
                        message_with_local_information->players_position[i][1] = htonl((uint32_t)k);

                  }else {
                  
                        message_with_local_information->players_position[i][0] = htonl((uint32_t)(size + 1));
                        message_with_local_information->players_position[i][1] = htonl((uint32_t)(size + 1));

                  }

            } else {
                  
                  message_with_local_information->players_position[i][0] = htonl((uint32_t)(size + 1));
                  message_with_local_information->players_position[i][1] = htonl((uint32_t)(size + 1));

            }
      }

      match_node->match->match_free = 1;
      pthread_cond_signal(&match_node->match->match_cond_var);
      pthread_mutex_unlock(&match_node->match->match_mutex);

      message_with_local_information->receiver_id = htonl((uint32_t)id_in_match);

      return message_with_local_information;


}

void send_global_map_to_client(int socket_for_thread, Match_list_node* match_node, User* current_user, int id_in_match) {
      
      uint32_t size = match_node->match->size;

      //Calculate total size of the message to send
      size_t header_size = sizeof(Global_Info_Header);
      size_t map_size = size * size * sizeof(char); 
      size_t total_message_size = header_size + map_size;

      //Allocates memory for the total message
      char* message_buffer = malloc(total_message_size);
      if (!message_buffer) {
            perror("malloc");
            handle_client_death_in_lobby(socket_for_thread, match_node, current_user, id_in_match);
      }

      //Build the info in the header
      Global_Info_Header header;
      header.size = htonl((uint32_t)size);
      header.receiver_id = htonl((uint32_t)id_in_match);

      pthread_mutex_lock(&match_node->match->match_mutex);

      while(match_node->match->match_free == 0)
            pthread_cond_wait(&match_node->match->match_cond_var, &match_node->match->match_mutex);

      match_node->match->match_free = 0;
      

      for(int i = 0; i < MAX_PLAYERS_MATCH; i++) {

            if(match_node->match->players[i] != NULL) {

                  header.players_position[i][0] = htonl((uint32_t)match_node->match->players[i]->x);
                  header.players_position[i][1] = htonl((uint32_t)match_node->match->players[i]->y);

            } else {
                  
                  header.players_position[i][0] = htonl((uint32_t)(size + 1));
                  header.players_position[i][1] = htonl((uint32_t)(size + 1));

            }
      }

      //Copies the header in the message_buffer
      memcpy(message_buffer, &header, header_size);

      //Copies the map in the message_buffer, not sending the walls
      int offset = header_size;
      for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                  
                  // If there is a wall ('w'), sends empty ('e')
                  if (match_node->match->map[i][j] == 'w') {
                        message_buffer[offset] = 'e';
                  } else {
                        //Otherwise sends whatever there is
                        message_buffer[offset] = match_node->match->map[i][j];
                  }
                  
                  offset++;
            }
      }


      match_node->match->match_free = 1;
      pthread_cond_signal(&match_node->match->match_cond_var);
      pthread_mutex_unlock(&match_node->match->match_mutex);

      
      send_all_in_match(socket_for_thread, message_buffer, total_message_size, match_node, current_user, id_in_match);

      
      free(message_buffer);

}

void handle_move_in_match(Match_list_node* match_node, int id_in_match, char option){

      int new_x = match_node->match->players[id_in_match]->x;
      int new_y = match_node->match->players[id_in_match]->y;

      switch (option){
            case 'w':
                  move(match_node, id_in_match, new_x - 1, new_y);
                  break;
            case 'a':
                  move(match_node, id_in_match, new_x, new_y - 1);
                  break;
            case 's':
                  move(match_node, id_in_match, new_x + 1, new_y);
                  break;
            case 'd':
                  move(match_node, id_in_match, new_x, new_y + 1);
                  break;
            
            default:
                  break;

      }
      
}

void move(Match_list_node* match_node, int id_in_match, int new_x, int new_y){

      int i;
      uint32_t size = match_node->match->size;
      int available_pos = 1;

      pthread_mutex_lock(&match_node->match->match_mutex);

      while(match_node->match->match_free == 0)
            pthread_cond_wait(&match_node->match->match_cond_var, &match_node->match->match_mutex);

      match_node->match->match_free = 0;

      if( (new_x >= 0 && new_x < size) && (new_y >= 0 && new_y < size)){

            if(match_node->match->map[new_x][new_y] != 'w'){

                  for(i = 0; i < MAX_PLAYERS_MATCH; i++)
                        if(i != id_in_match && match_node->match->players[i] != NULL)
                              if(match_node->match->players[i]->x == new_x && match_node->match->players[i]->y == new_y){
                                    available_pos = 0;
                                    break;
                              }
                  
            }else{
                  available_pos = 0;
            }

      }else{
            available_pos = 0;
      }

      if(available_pos == 1){

            match_node->match->players[id_in_match]->x = new_x;
            match_node->match->players[id_in_match]->y = new_y;
            
            match_node->match->map[new_x][new_y] = colors[id_in_match];
      }


      match_node->match->match_free = 1;
      pthread_cond_signal(&match_node->match->match_cond_var);
      pthread_mutex_unlock(&match_node->match->match_mutex);

}

void handle_match_ending(int socket_for_thread, Match_list_node* match_node, User* current_user, int id_in_match){

      char* ending_message = malloc(sizeof(char) * 1024);

      if (!ending_message){

            perror("malloc");
            handle_client_death_in_lobby(socket_for_thread, match_node, current_user, id_in_match);

      }

      int num_cells_for_player[MAX_PLAYERS_MATCH] = {0};

      int i, j;
      uint32_t size = match_node->match->size;
      int max = -1, winner = 0, draw = 0;

      sleep(1);         //To be sure every player has finished

      //Doesn't lock the mutex to let all players threads read data synchronous, it's only read, no one will modify

      for(i = 0; i < size; i++){

            for(j = 0; j < size; j++){

                  char current_cell = match_node->match->map[i][j];

                  if (current_cell == colors[0]) {

                        num_cells_for_player[0]++;

                  } else if (current_cell == colors[1]) {

                        num_cells_for_player[1]++;

                  } else if (current_cell == colors[2]) {

                        num_cells_for_player[2]++;

                  } else if (current_cell == colors[3]) {

                        num_cells_for_player[3]++;
                        
                  }


            }


      }

      strcpy(ending_message, "\nLa partita è terminata!\nEcco i risultati:\n\n");

      //Now locks the mutex for this final part, in case some clients disconnect in the process
      pthread_mutex_lock(&match_node->match->match_mutex);

      while(match_node->match->match_free == 0)
            pthread_cond_wait(&match_node->match->match_cond_var, &match_node->match->match_mutex);

      match_node->match->match_free = 0;

      for(i = 0; i < MAX_PLAYERS_MATCH; i++){
            if(match_node->match->players[i] != NULL){
                  strcat(ending_message, "Giocatore ");
                  strcat(ending_message, match_node->match->players[i]->username);
                  strcat(ending_message, "\tPunti : ");

                  char string_number[10] = {0};

                  sprintf(string_number, "%d", num_cells_for_player[i]);

                  strcat(ending_message, string_number);

                  strcat(ending_message, "\n");

                  if(num_cells_for_player[i] > max){

                        max = num_cells_for_player[i];
                        winner = i;
                        draw = 0;

                  }else if(num_cells_for_player[i] == max){

                        draw = 1;

                  }
                        

            }
      }

      match_node->match->status = NOT_IN_PROGRESS;

      if(draw == 0){

            strcat(ending_message, "\nIl vincitore è il giocatore ");
            strcat(ending_message, match_node->match->players[winner]->username);

      }else{

            strcat(ending_message, "\nC'e' stato un pareggio, i vincitori sono i giocatori:\n\n");
            for(i = 0; i < MAX_PLAYERS_MATCH; i++){
                  if(match_node->match->players[i] != NULL)
                        if(num_cells_for_player[i] == max){
                              strcat(ending_message, match_node->match->players[i]->username);
                              strcat(ending_message, "\n");
                        }
            }


      }

      match_node->match->match_free = 1;
      pthread_cond_signal(&match_node->match->match_cond_var);
      pthread_mutex_unlock(&match_node->match->match_mutex);

      send_all_in_match(socket_for_thread, ending_message, strlen(ending_message) + 1, match_node, current_user, id_in_match);

      free(ending_message);

      //handle_being_in_lobby(socket_for_thread, match_node, current_user, id_in_match);
      //not needed thanks to the returns

}

/*
void siguser1_handler(int signal){

      int i;

      int* socket_for_thread = pthread_getspecific(key_for_socket);
      User* current_user = NULL;
      int id_in_match;

      Match_list_node* tmp = match_list;

      int player_found = 0;
      while(tmp != NULL && player_found == 0){

            for(i=0; i < MAX_PLAYERS_MATCH; i++)
                  if(tmp->match->players[i] != NULL)
                        if(tmp->match->players[i]->tid == pthread_self()){

                              current_user = malloc(sizeof(User));
                              strcpy(current_user->username, tmp->match->players[i]->username);

                              id_in_match = i;

                              player_found = 1;
                              break;

                        }

            if(player_found == 1)
                  break;
            else
                  tmp = tmp->next;
      }

      if(player_found == 1)
            handle_being_in_match(*socket_for_thread, tmp, current_user, id_in_match);

}
*/

Match* init_match(int socket_for_thread, User* creator, uint32_t size){
      
      Match* match_created = malloc(sizeof(Match));
      
      if (!match_created) {

            perror("malloc");
            close(socket_for_thread);

            pthread_exit(0);
      }
      
      //Assigning id
      pthread_mutex_lock(&next_match_id_mutex);

      while(next_match_id_free == 0)
            pthread_cond_wait(&next_match_id_cond_var, &next_match_id_mutex);

      next_match_id_free = 0;

      match_created->id = next_match_id;
      next_match_id++;

      next_match_id_free = 1;
      pthread_cond_signal(&next_match_id_cond_var);
      pthread_mutex_unlock(&next_match_id_mutex);

      //Putting the creator in players
      User_in_match* new_player = malloc(sizeof(User_in_match));
      strcpy(new_player->username, creator->username);
      new_player->tid = pthread_self();
      match_created->players[0] = new_player;

      //Other pointers in the arrray are NULL, because there are no other players yet
      int i;
      for(i=1; i < MAX_PLAYERS_MATCH; i++)
            match_created->players[i] = NULL;


      //Putting the creator as the host
      match_created->host = 0;

      //Putting the status as NOT_IN_PROGRESS
      match_created->status = NOT_IN_PROGRESS;

      //The chosen size
      match_created->size = size;

      //Allocating the map
      match_created->map = malloc(size*sizeof(char*));

      if (!(match_created->map)) {

            perror("malloc");
            close(socket_for_thread);

            pthread_exit(0);
      }

      for(i=0; i < size; i++){

            match_created->map[i] = malloc(size*sizeof(char));
            if (!(match_created->map[i])) {

                  perror("malloc");
                  close(socket_for_thread);

                  pthread_exit(0);
            }

      }

      //Initializing mutex and condition variables
      pthread_mutex_init(&match_created->match_mutex, NULL);
      pthread_cond_init(&match_created->match_cond_var, NULL);
      match_created->match_free = 1;


      return match_created;
}

void free_match(Match* match){

      if (match == NULL) {
            return;
      }

      int i;
      //Deallocating players
      for(i = 0; i < MAX_PLAYERS_MATCH; i++)
            free(match->players[i]);

      //Deallocating the map
      for(i = 0; i < match->size; i++)
            free(match->map[i]);
      
      free(match->map);

      //Deallocating mutex and condition variables
      pthread_mutex_destroy(&match->match_mutex);
      pthread_cond_destroy(&match->match_cond_var);

      free(match);
      
}

static void* handle_client(void* arg){

      int socket_for_thread = *(int *) arg;
      free(arg);

      /*
      int* ptr_socket_for_thread = malloc(sizeof(int));
      *ptr_socket_for_thread = socket_for_thread;
      

      pthread_setspecific(key_for_socket, ptr_socket_for_thread);

      */

      User* current_user = handle_starting_interaction(socket_for_thread);

      if(current_user != NULL)
            handle_session(socket_for_thread, current_user);


      
      /*
      char buf[1024];
      for (;;) {

            ssize_t n = recv(socket_for_thread, buf, sizeof(buf), 0);

            if (n == 0) 
                  break;                  // client ha chiuso

            if (n < 0) { 

                  if (errno == EINTR) 
                        continue; 
                  
                  perror("recv"); 
                  break; 

            }

            // echo
            ssize_t off = 0;
            sleep(5);

            while (off < n) {

                  ssize_t w = send(socket_for_thread, buf + off, (size_t)(n - off), 0);

                  if (w < 0) { 

                        if (errno == EINTR) 
                              continue; 
                  
                        if (errno == EPIPE) {
                              printf("Client morto\n");
                              close(socket_for_thread);
                              pthread_exit(0);
                        }

                        perror("send"); 
                        close(socket_for_thread); 
                        _exit(1); 
                  }

                  off += w;

            }

      }

      close(socket_for_thread);
      return NULL;

      */

      /*
      time_t last = time(NULL);

            for (;;) {
                  char buf[MAXLINE];

                  // (1) prova a leggere senza bloccare
                  ssize_t n = recv(connection_socket, buf, sizeof(buf), MSG_DONTWAIT);

                  if (n > 0) {
                  // echo
                  send(connection_socket, buf, n, 0);

                  } else if (n == 0) {
                        // client chiuso
                        printf("Client ha chiuso.\n");
                        break;

                  } else {
                        // n < 0
                        if (errno != EAGAIN && errno != EWOULDBLOCK) {
                              // errore reale
                              perror("recv");
                              break;
                        }
                        // altrimenti non c'è nessun dato, tutto normale
                  }

                  // (2) invio periodico ogni 10s
                  time_t now = time(NULL);
                  if (now - last >= 10) {
                  const char *msg = "x\n";
                  ssize_t w = send(connection_socket, msg, strlen(msg), 0);
                  
                  if (w < 0) { 

                        if (errno == EINTR) 
                              continue; 

                        if (errno == EPIPE){
                              printf("Client morto\n");
                              close(connection_socket);
                              
                        }

                        perror("send"); 
                        close(connection_socket); 
                  }

                  last = now;

                  }

                  // (3) pausa minima per non saturare la CPU
                  usleep(10000); // 10ms
            }

            close(connection_socket);

            */

      //free(ptr_socket_for_thread);

      return NULL;

}

