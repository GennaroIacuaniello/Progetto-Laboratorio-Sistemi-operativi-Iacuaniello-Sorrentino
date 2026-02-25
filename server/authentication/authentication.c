#include "authentication.h"

#include "network.h"
#include "session.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <unistd.h>


//Variables used to control access to users file "Database" (in contains couples of data username-hashed_password)
static pthread_mutex_t users_file_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t users_file_cond_var = PTHREAD_COND_INITIALIZER;    
static int users_file_free = 1;

unsigned long hash(unsigned char *str){

    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
            hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;

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

