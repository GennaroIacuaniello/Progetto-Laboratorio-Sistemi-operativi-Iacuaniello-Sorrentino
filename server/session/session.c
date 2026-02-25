#include "session.h"

#include "network.h"
#include "match.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>

//Global list of matches
Match_list_node* match_list = NULL;

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

