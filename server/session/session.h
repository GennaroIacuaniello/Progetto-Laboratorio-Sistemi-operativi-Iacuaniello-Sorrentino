#ifndef SESSION_H
#define SESSION_H

#define MAX_PLAYERS_MATCH 4

#include "authentication.h"

#include <sys/types.h>
#include <arpa/inet.h>
#include <pthread.h>

typedef struct User_in_match{

      char username[64];
      pthread_t tid;                //tid of the thread of the user, used to send signals
      unsigned int x;
      unsigned int y;

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

extern Match_list_node* match_list;

void handle_session(int socket_for_thread, User* current_user);
void lobby_creation(int socket_for_thread, User* current_user);
void join_lobby(int socket_for_thread, User* current_user);
char* get_message_with_matches_info(int socket_for_thread, int error_flag);
unsigned int join_spcific_lobby(int socket_for_thread, User* current_user, Match_list_node* match_to_join);

void handle_being_in_lobby(int socket_for_thread, Match_list_node* match_node, User* current_user, int id_in_match);
char* get_message_list_of_players_and_size(int socket_for_thread, User* current_user, Match_list_node* match_node, int id_in_match);

void change_map_size(int socket_for_thread, Match_list_node* match_node, User* current_user, int id_in_match);

void handle_leaving_lobby(Match_list_node* match_node, int id_in_match);
void check_host_change_or_match_cancellation(Match_list_node* match_node, int previous_host_id_in_match);

void handle_client_death_in_lobby(int socket_for_thread, Match_list_node* match_node, User* current_user, int id_in_match);


#endif