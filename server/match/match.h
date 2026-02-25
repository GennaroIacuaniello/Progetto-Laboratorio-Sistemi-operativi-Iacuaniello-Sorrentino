#ifndef MATCH_H
#define MATCH_H


#include "session.h" 

#include <arpa/inet.h>

#define SECONDS_PER_SIZE 120        //Match lenght starts from 2 minutes (120 seconds) and scales with the map size (16x16 120 sec, 32x32 240 sec, 48x48 360 sec)
#define TIMER_GLOBAL_MAP 30         //Interval of seconds every time the server sends the global map


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


void start_match(int socket_for_thread, Match_list_node* match_node, User* current_user, int id_in_match);

void handle_being_in_match(int socket_for_thread, Match_list_node* match_node, User* current_user, int id_in_match);

Message_with_local_information* get_message_with_local_information(Match_list_node* match_node, int id_in_match);
void send_global_map_to_client(int socket_for_thread, Match_list_node* match_node, User* current_user, int id_in_match);

void handle_move_in_match(Match_list_node* match_node, int id_in_match, char option);
void move(Match_list_node* match_node, int id_in_match, int new_x, int new_y);

void handle_match_ending(int socket_for_thread, Match_list_node* match_node, User* current_user, int id_in_match);


Match* init_match(int socket_for_thread, User* creator, uint32_t size);
void free_match(Match* match);

#endif