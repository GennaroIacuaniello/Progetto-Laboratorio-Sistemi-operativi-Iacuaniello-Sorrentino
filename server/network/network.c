#include "network.h"

#include "authentication.h"
#include "session.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

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
