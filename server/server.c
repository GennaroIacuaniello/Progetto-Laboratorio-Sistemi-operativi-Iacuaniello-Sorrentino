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

      unsigned char username[64];
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

      unsigned char username[64];
      unsigned int x;
      unsigned int y;
      //unsigned int num_colored_cells;

}User_in_match;

typedef struct Match{

      unsigned long long id;                          //id of the lobby
      User_in_match* players[MAX_PLAYERS_MATCH];       //array of players, up to MAX_PLAYERS_MATCH players per match
      int host;                                       //Index of the array players that indicates which one is the host of the game
      int size;                                       //Map size
      char** map;                                     //Map of the game

      pthread_mutex_t map_mutex;
      pthread_cond_t map_cond_var;    
      int map_free;

}Match;

typedef struct Match_list_node{

      Match* match;
      struct Match_list_node* next;

}Match_list_node;

//Global list of matches
Match_list_node* match_list = NULL;




//Variables used to give ids to matches
pthread_mutex_t next_match_id_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t next_match_id_cond_var = PTHREAD_COND_INITIALIZER;    
int next_match_id_free = 1;

unsigned long long next_match_id = 1;




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
      },

      {
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
      }

};

//Variables used to control access to users file "Database" (in contains couples of data username-hashed_password)
pthread_mutex_t users_file_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t users_file_cond_var = PTHREAD_COND_INITIALIZER;    
int users_file_free = 1;


User* handle_starting_interaction(int socket_for_thread);
User* handle_login(int socket_for_thread);
User* handle_registration(int socket_for_thread);


unsigned int login(unsigned char* username, unsigned char* password);
unsigned int registration(unsigned char* username, unsigned char* password);
unsigned long hash(unsigned char *str);


void handle_session(int socket_for_thread, User* current_user);
void lobby_creation(int socket_for_thread, User* current_user);
void join_lobby(int socket_for_thread, User* current_user);

void handle_being_in_lobby(int socket_for_thread, User* current_user, int id_in_match);


Match* init_match(int socket_for_thread, User* creator, int size);

ssize_t send_all(int socket_for_thread, const void* buf, size_t n);
ssize_t recv_all(int socket_for_thread, void* buf, size_t n, int flags);


static void* handle_client(void* arg);

//TODO Rimuovere le seguenti funzioni di debug
void read_users(){
      
      FILE* file_users = fopen("users.dat", "rb+");

      if(file_users != NULL){
            
            User current_user;

            while(fread(&current_user, sizeof(User), 1, file_users) == 1){

                  printf("%s, %ld", current_user.username, current_user.hashed_password);
                        

            }

            
      }else{
            perror("fopen");
            
      }

}


int main(int argc, char* argv[]){

      signal(SIGPIPE, SIG_IGN);

      srand(time(NULL));

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



      return 0;
}










unsigned int login(unsigned char* username, unsigned char* password){


      pthread_mutex_lock(&users_file_mutex);

      while(users_file_free == 0)
            pthread_cond_wait(&users_file_cond_var, &users_file_mutex);

      users_file_free = 0;

      FILE* file_users = fopen("users.dat", "rb");
      unsigned long hashed_password = hash(password);

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

unsigned int registration(unsigned char* username, unsigned char* password){

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
            unsigned long hashed_password = hash(password);
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

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;

}


ssize_t send_all(int socket_for_thread, const void* buf, size_t n){

      size_t sent = 0;
      const void* ptr = buf;

      while(sent < n){

            ssize_t w = send(socket_for_thread, ptr + sent, (size_t)(n - sent), 0);

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

            sent += (size_t)w;
      }

      return (ssize_t)sent;
      
}

ssize_t recv_all(int socket_for_thread, void* buf, size_t n, int flags){
      size_t received = 0;
      void* ptr = buf;

      while(received < n){

            ssize_t r = recv(socket_for_thread, ptr + received, (size_t)(n - received), flags);

            if( r < 0 ){

                  if(errno = EINTR)
                        continue;
                  
                  perror("recv"); 
                  close(socket_for_thread); 
                  _exit(1); 

            }

            if( r == 0 )       //connessione chiusa
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
                        char closing_message[] = "\nArrivederci!";
                        send_all(socket_for_thread, closing_message, sizeof(closing_message));

                        close(socket_for_thread);

                        pthread_exit(0);

                  default:
                        char error_message[] = "\nOpzione non valida!\nInserire il numero relativo all'opzione che si desidera:\n1)Login\n2)Registrazione\n3)Esci\n";
                        send_all(socket_for_thread, error_message, sizeof(error_message));
                        break;
            }

      }
      
      

}

User* handle_login(int socket_for_thread){

      char username_request[] = "\nInserire il proprio username: ";
      char password_request[] = "\nInserire la propria password: ";
      char username[64], password[64];

      send_all(socket_for_thread, username_request, sizeof(username_request));

      recv_all(socket_for_thread, username, sizeof(username), 0);

      send_all(socket_for_thread, password_request, sizeof(password_request));

      recv_all(socket_for_thread, password, sizeof(password), 0);

      unsigned int res = login(username, password);

      if(res == 0){
            
            User* current_user = malloc(sizeof(User));

            if (!current_user) {

                  perror("malloc");
                  close(socket_for_thread);

                  pthread_exit(0);

            }

            strcpy(current_user->username, username);

            char login_success_message[] = "\nLogin effettuato con successo!\n";
            send_all(socket_for_thread, login_success_message, sizeof(login_success_message));

            return current_user;

      }else{

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
                              char closing_message[] = "\nArrivederci!";
                              send_all(socket_for_thread, closing_message, sizeof(closing_message));

                              close(socket_for_thread);

                              pthread_exit(0);

                        default:
                              char error_message[] = "\nOpzione non valida!\nInserire il numero relativo all'opzione che si desidera:\n1)Login\n2)Registrazione\n3)Esci\n";
                              send_all(socket_for_thread, error_message, sizeof(error_message));
                              break;
                  }

            }
            


      }

}

User* handle_registration(int socket_for_thread){

      char username_request[] = "\nInserire il proprio username: ";
      char password_request[] = "\nInserire la propria password: ";
      char username[64], password[64];

      send_all(socket_for_thread, username_request, sizeof(username_request));

      recv_all(socket_for_thread, username, sizeof(username), 0);

      send_all(socket_for_thread, password_request, sizeof(password_request));

      recv_all(socket_for_thread, password, sizeof(password), 0);

      unsigned int res = registration(username, password);

      if(res == 0){
            
            User* current_user = malloc(sizeof(User));

            if (!current_user) {

                  perror("malloc");
                  close(socket_for_thread);

                  pthread_exit(0);

            }

            strcpy(current_user->username, username);

            char registration_success_message[] = "\nRegistrazione effettuata con successo!\n";
            send_all(socket_for_thread, registration_success_message, sizeof(registration_success_message));

            return current_user;

      }else {

            char registration_username_error_message[] = "\nRegistrazione fallita perchè lo username è già in uso!\nCosa si desidera fare?\n1)Riprova\n2)Effettuare il login\n3)Esci\n";
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
                              char closing_message[] = "\nArrivederci!";
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

}


void handle_session(int socket_for_thread, User* current_user){

      char welcome_message_and_menu[] = "\n        DUNGEONS & COLORS        \n\nBenvenuti nel gioco!\nInserire il numero relativo all'opzione che si desidera:\n\1)Crea lobby\n2)Entra in una lobby\n3)Esci\n";

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
                        char closing_message[] = "\nArrivederci!";
                        send_all(socket_for_thread, closing_message, sizeof(closing_message));
                        
                        free(current_user);
                        close(socket_for_thread);

                        pthread_exit(0);

                  default:
                        char error_message[] = "\nOpzione non valida!\nInserire il numero relativo all'opzione che si desidera:\n\1)Crea lobby\n2)Entra in una lobby\n3)Esci\n";
                        send_all(socket_for_thread, error_message, sizeof(error_message));
                        break;
            }

      }

}

void lobby_creation(int socket_for_thread, User* current_user){

      char size_request[] = "\nInserire l'opzione relativa alla dimensione della mappa con cui si desidera giocare:\n1)16x16\n2)32x32\n3)48x48\n";

      send_all(socket_for_thread, size_request, sizeof(size_request));

      int size;
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

      if(match_list == NULL){
            Match_list_node* new_node = malloc(sizeof(Match_list_node));
            new_node->match = new_match;
            new_node->next = NULL;
            match_list = new_node;
      }else{
            Match_list_node* tmp = malloc(sizeof(Match_list_node));
            tmp->match = new_match;
            tmp->next = match_list;
            match_list = tmp;
      }

      handle_being_in_lobby(socket_for_thread, current_user, 0);      //0 as id_in_match because the lobby has been just created by this user, so he is the only user and also the host

}

void join_lobby(int socket_for_thread, User* current_user){

}

void handle_being_in_lobby(int socket_for_thread, User* current_user, int id_in_match){





}


Match* init_match(int socket_for_thread, User* creator, int size){

      Match* match_created = malloc(sizeof(Match));

      //Assigning id
      pthread_mutex_lock(&next_match_id_mutex);

      while(next_match_id_free == 0)
            pthread_cond_wait(&next_match_id_cond_var, &next_match_id_mutex);

      users_file_free = 0;

      match_created->id = next_match_id;
      next_match_id++;

      users_file_free = 1;
      pthread_cond_signal(&next_match_id_cond_var);
      pthread_mutex_unlock(&next_match_id_mutex);

      //Putting the creator in players
      User_in_match* new_player = malloc(sizeof(User_in_match));
      strcpy(new_player->username, creator->username);
      match_created->players[0] = new_player;

      //Other pointers in the arrray are NULL, because there are no other players yet
      int i;
      for(i=1; i < MAX_PLAYERS_MATCH; i++)
            match_created->players[i] = NULL;


      //Putting the creator as the host
      match_created->host = 0;

      //The chosen size
      match_created->size = size;

      //Allocating the map
      match_created->map = malloc(size*sizeof(char));

      if (!(match_created->map)) {

            perror("malloc");
            close(socket_for_thread);

            pthread_exit(0);
      }

      int i, j, x, y, z, k;
      for(i=0; i < size; i++){

            match_created->map[i] = malloc(size*sizeof(char));
            if (!(match_created->map)) {

                  perror("malloc");
                  close(socket_for_thread);

                  pthread_exit(0);
            }

      }

      //Random creation of the map
      int num_of_blocks = size/16;        //Number of 16x16 blocks of the map
      for(i=0; i < num_of_blocks; i++){

            for(j=0; j < num_of_blocks; j++){
                  
                  int random_preset = rand() % 4;
                  z = 0;
                  k = 0;

                  for(x = i*16; x < (i+1)*16; x++, z++){

                        for(y = j*16; y < (j+1)*16; y++, k++){

                              match_created->map[x][y] = array_of_map_presets[random_preset][z][k];

                        }


                  }


            }


      }



      //Initializing mutex and condition variables
      pthread_mutex_init(&match_created->map_mutex, NULL);
      pthread_cond_init(&match_created->map_cond_var, NULL);
      match_created->map_free = 1;


      return match_created;
}

void free_match(Match* match){

      //Deallocating the map
      int i;
      for(i = 0; i < match->size; i++)
            free(match->map[i]);
      
      free(match->map);

      //Deallocating mutex and condition variables
      pthread_mutex_destroy(&match->map_mutex);
      pthread_cond_destroy(&match->map_cond_var);

      free(match);
      
}

static void* handle_client(void* arg){

      int socket_for_thread = *(int *) arg;
      free(arg);

      User* current_user = handle_starting_interaction(socket_for_thread);


      
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

      return NULL;

}

