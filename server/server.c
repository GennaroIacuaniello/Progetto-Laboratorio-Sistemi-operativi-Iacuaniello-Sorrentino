//Compile: gcc server.c authentication/authentication.c network/network.c session/session.c match/match.c -I./authentication -I./network -I./session -I./match -o server.out -lpthread
//Run: ./server.out

#include "authentication/authentication.h"
#include "network/network.h"
#include "session/session.h"
#include "match/match.h"

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


User* handle_starting_interaction(int socket_for_thread);

static void* handle_client(void* arg);


int main(int argc, char* argv[]){

      signal(SIGPIPE, SIG_IGN);
      
      srand(time(NULL));

      //If it doesn't already exist, creates the "database" file of users
      int fd = open("users.dat", O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
      if(fd >= 0)
            close(fd);
      else{
            perror("open"); 
            exit(1);
      }

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




static void* handle_client(void* arg){

      int socket_for_thread = *(int *) arg;
      free(arg);

      User* current_user = handle_starting_interaction(socket_for_thread);

      if(current_user != NULL)
            handle_session(socket_for_thread, current_user);


      return NULL;

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
