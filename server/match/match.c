#include "match.h"
#include "network.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <time.h>
#include <arpa/inet.h>
#include <pthread.h>



static const unsigned char colors[MAX_PLAYERS_MATCH] = {'b', 'r', 'g', 'y'};      //Global variable used for player colors


static const unsigned char array_of_map_presets[4][16][16] = {

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


//Variables used to give ids to matches
static pthread_mutex_t next_match_id_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t next_match_id_cond_var = PTHREAD_COND_INITIALIZER;    
static int next_match_id_free = 1;

static uint32_t next_match_id = 1;


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

      strcat(ending_message, "\n\nTra 30 secondi si ritornera' alla lobby!\n");

      send_all_in_match(socket_for_thread, ending_message, strlen(ending_message) + 1, match_node, current_user, id_in_match);

      free(ending_message);

      //handle_being_in_lobby(socket_for_thread, match_node, current_user, id_in_match);
      //not needed thanks to the returns

}


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


