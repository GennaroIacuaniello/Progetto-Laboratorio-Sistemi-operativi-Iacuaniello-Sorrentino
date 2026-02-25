//Compile: gcc -o client.out client.c
//Run: ./client IP PORT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>    //To handle "moving in the map" during the game
#include <errno.h>
#include <signal.h>
//#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <limits.h>

//Max messages lenght
#define MAX_LENGHT 4096

//Commands to see the map full screen
#define ENTER_ALT_SCREEN "\033[?1049h" // Entra nello schermo intero
#define EXIT_ALT_SCREEN  "\033[?1049l" // Torna al terminale normale
#define HIDE_CURSOR      "\033[?25l"   // Nasconde il cursore lampeggiante
#define SHOW_CURSOR      "\033[?25h"   // Mostra di nuovo il cursore

//Clear commands
#define CLEAR_SCREEN     "\033[2J"     // Pulisce tutto lo schermo
#define CURSOR_HOME      "\033[H"      // Riporta il cursore in alto a sinistra (0,0)

//Reset to go back to normal terminal background color
#define RESET       "\033[0m"

//Background colors
#define BG_BLACK    "\033[40m"   //For empty cells ('e')
#define BG_WHITE    "\033[47m"   //For walls ('w')
#define BG_BLUE     "\033[44m"   //Player 0 ('b')
#define BG_RED      "\033[41m"   //Player 1 ('r')
#define BG_GREEN    "\033[42m"   //Player 2 ('g')
#define BG_YELLOW   "\033[43m"   //Player 3 ('y')

#define FG_WHITE_BOLD "\033[1;37m"
#define FG_BLACK_BOLD "\033[1;30m"

struct termios orig_termios;        //Used to manipulate input reading during a match

#define MAX_PLAYERS_MATCH 4

//Array of players backgrond colors
const char* bg_colors[4] = {
    "\033[44m", //0: Blue (b)
    "\033[41m", //1: Red (r)
    "\033[42m", //2: Green (g)
    "\033[43m"  //3: yellow (y)
};

typedef struct Message_with_local_information{

    uint32_t players_position[MAX_PLAYERS_MATCH][2];        //Every row has 2 cells, if a player is in the local map, its cells are not size of the map + 1

    uint32_t my_id;                                         //Id_in_match of the player who is receiving the message, used to know its data from the array players_position 

    char local_map[5][5];

    char message[25];

}Message_with_local_information;

typedef struct Global_Info_Header{
      
    uint32_t players_position[MAX_PLAYERS_MATCH][2];      //Every row has 2 cells, if a player is in the global map, its cells are not size of the map + 1

    uint32_t my_id;                                       //Id_in_match of the player who is receiving the message, used to know its data from the array players_position 

    uint32_t size;                                        //Global map size

} Global_Info_Header;


void check_terminal_size();         //Initial function to warn the user in case of too litle terminal

void enable_terminal_game_mode();       //Enables "game mode" in the terminal
void disable_terminal_game_mode();      //Disables "game mode" in the terminal

ssize_t send_all(int socket_fd, const void* buf, size_t n);
ssize_t recv_all(int socket_fd, void* buf, size_t n, int flags);

void handle_starting_interaction(int socket_fd);
void handle_login(int socket_fd);
void handle_registration(int socket_fd);

void handle_session(int socket_fd);
void lobby_creation(int socket_fd);
void join_lobby(int socket_fd);

void handle_being_in_lobby(int socket_fd);
void handle_change_map_size(int socket_fd);

void handle_being_in_match(int socket_fd);
void print_map(uint32_t players_positions[MAX_PLAYERS_MATCH][2], uint32_t my_id, char** map, uint32_t size);
void handle_local_info(int socket_fd, char** map, uint32_t size);
void handle_global_info(int socket_fd, char** map, uint32_t size);

void handle_match_ending(int socket_fd);



int main(int argc, char* argv[]) {

    signal(SIGPIPE, SIG_IGN);
    tcgetattr(STDIN_FILENO, &orig_termios);

    //Initial terminal size check
    check_terminal_size();

    int socket_fd;
    struct sockaddr_in server_addr;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <server_ip> <port>\n", argv[0]);
        exit(1);
    }

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("socket");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(atoi(argv[2]));

    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        exit(1);
    }

    if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        exit(1);
    }

    printf("Connesso al server %s:%s\n", argv[1], argv[2]);
    sleep(1);

    //Entering "full screen mode"
    printf("\033[?1049h"); // ENTER_ALT_SCREEN
    printf("\033[2J\033[H"); // CLEAR_SCREEN and CURSOR_HOME to put the cursor at the top left corner of the screen
    fflush(stdout); //To forse the start of the "mode"

    handle_starting_interaction(socket_fd);


    return 0;
}







void check_terminal_size() {

    struct winsize w;

    //Do nothing if ioctl fails ( TIOCGWINSZ = Terminal IOCtl Get WINdow SiZe)
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) {
        return;
    }

    //50 rows guarantees space for the biggest map (48x48 + border/message)
    if (w.ws_row <= 50 || w.ws_col < 100) {
        printf("\n======================================================\n");
        printf("[ATTENZIONE] Il tuo terminale è troppo piccolo!\n");
        printf("Attuale: %d righe x %d colonne.\n", w.ws_row, w.ws_col);
        printf("Consigliato: almeno 51 righe x 100 colonne.\n\n");
        printf("Per visualizzare correttamente la mappa di gioco più grande (48x48),\n");
        printf("ti consigliamo di ingrandire la finestra del terminale a tutto schermo\n");
        printf("oppure di rimpicciolire leggermente il font (usa Ctrl e il tasto '-' o '+').\n");
        printf("======================================================\n\n");
        
        printf("Premi INVIO per continuare comunque con la connessione...");
        getchar();
    }

}


ssize_t send_all(int socket_fd, const void* buf, size_t n){

      size_t sent = 0;
      const char* ptr = (const char*)buf;

      while(sent < n){

            ssize_t w = send(socket_fd, ptr + sent, (size_t)(n - sent), 0);

            if (w < 0) { 

                  if (errno == EINTR) 
                        continue; 
            
                  if (errno == EPIPE) {
                        perror("Server morto\n");
                        close(socket_fd);
                        exit(1);
                  }

                  perror("send"); 
                  close(socket_fd); 
                  exit(1);
            }

            sent += (size_t)w;
      }

      return (ssize_t)sent;
      
}

ssize_t send_all_in_match(int socket_fd, const void* buf, size_t n){

      size_t sent = 0;
      const char* ptr = (const char*)buf;

      while(sent < n){

            ssize_t w = send(socket_fd, ptr + sent, (size_t)(n - sent), 0);

            if (w < 0) { 

                if (errno == EINTR) 
                    continue; 
        
                if (errno == EPIPE) {
                    perror("Server morto\n");
                    close(socket_fd);
                    sleep(2);
                    disable_terminal_game_mode();
                    printf("\033[?1049l"); // EXIT_ALT_SCREEN
                    fflush(stdout);
                    exit(1);
                }

                perror("send"); 
                close(socket_fd);
                sleep(2);
                disable_terminal_game_mode();
                printf("\033[?1049l"); // EXIT_ALT_SCREEN
                fflush(stdout);
                exit(1);

            }

            sent += (size_t)w;
      }

      return (ssize_t)sent;
      
}

ssize_t recv_all(int socket_fd, void* buf, size_t n, int flags){

      size_t received = 0;
      char* ptr = (char*)buf;

      while(received < n){

            ssize_t r = recv(socket_fd, ptr + received, (size_t)(n - received), flags);

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
                close(socket_fd); 
                exit(1); 

            }

            if( r == 0 )       //connection closed
                  return (ssize_t)received;


            received += (size_t)r;


      }

      return (ssize_t)received;

}

ssize_t recv_all_in_match(int socket_fd, void* buf, size_t n, int flags){

      size_t received = 0;
      char* ptr = (char*)buf;

      while(received < n){

            ssize_t r = recv(socket_fd, ptr + received, (size_t)(n - received), flags);

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
                close(socket_fd);
                sleep(2);
                disable_terminal_game_mode();
                printf("\033[?1049l"); // EXIT_ALT_SCREEN
                fflush(stdout);
                exit(1); 

            }

            if( r == 0 )       //connection closed
                  return (ssize_t)received;


            received += (size_t)r;


      }

      return (ssize_t)received;

}

ssize_t recv_string(int socket_fd, char* buf, size_t max_len, int flags) {

    size_t received = 0;
    char c;

    //Read until the max is reached, leaving 1 place for string terminator '\0'
    while (received < max_len - 1) {
        
        ssize_t r = recv(socket_fd, &c, (size_t)1, flags); // Reads 1 char

        if (r < 0) {

            if (errno == EINTR) 
                continue;
            
            
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                if (received == 0) {

                    return -1; 

                } else {
                    
                    usleep(5000);   //5 ms

                    continue;
                }
            }

            //Real error
            perror("recv");
            close(socket_fd);
            exit(1);
            
        }

        if (r == 0) {
            //connection closed
            printf("\nConnessione chiusa dal server.\n");
            exit(0);
        }

        buf[received] = c;
        received++;

        //Stops if receives string terminator '\0'
        if (c == '\0') {
            break;
        }
    }

    buf[received] = '\0';

    return received;

}

ssize_t recv_string_in_match(int socket_fd, char* buf, size_t max_len, int flags) {

    size_t received = 0;
    char c;

    //Read until the max is reached, leaving 1 place for string terminator '\0'
    while (received < max_len - 1) {
        
        ssize_t r = recv(socket_fd, &c, (size_t)1, flags); // Reads 1 char

        if (r < 0) {

            if (errno == EINTR) 
                continue;
            
            
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                if (received == 0) {

                    return -1; 

                } else {
                    
                    usleep(5000);   //5 ms

                    continue;
                }
            }

            //Real error
            perror("recv");
            close(socket_fd);
            sleep(2);
            disable_terminal_game_mode();
            printf("\033[?1049l"); // EXIT_ALT_SCREEN
            fflush(stdout);
            exit(1);
            
        }

        if (r == 0) {
            //connection closed
            printf("\nConnessione chiusa dal server.\n");
            sleep(2);
            disable_terminal_game_mode();
            printf("\033[?1049l"); // EXIT_ALT_SCREEN
            fflush(stdout);
            exit(0);
        }

        buf[received] = c;
        received++;

        //Stops if receives string terminator '\0'
        if (c == '\0') {
            break;
        }
    }

    buf[received] = '\0';

    return received;

}

void handle_starting_interaction(int socket_fd){

    char start_message[MAX_LENGHT];

    recv_string(socket_fd, start_message, MAX_LENGHT, 0);

    int done = 0;

    while (done == 0){

        //CLEAR_SCREEN and CURSOR_HOME to put the cursor at the top left corner of the screen
        printf("\033[2J\033[H");

        printf("%s", start_message);

        uint32_t option;
        uint32_t option_to_send;

        if (scanf("%u", &option) != 1) {
            option = UINT_MAX;     //Garbage value to send and so receive "Option not valid"
        }

        //Cleans the buffer
        int c;
        while ((c = getchar()) != '\n' && c != EOF);


        option_to_send = htonl(option);


        send_all(socket_fd, &option_to_send, sizeof(option_to_send));

        switch (option){
                case 1:
                    done = 1;
                    handle_login(socket_fd);
                    return;
                case 2:
                    done = 1;
                    handle_registration(socket_fd);
                    return;
                case 3:
                    done = 1;

                    recv_string(socket_fd, start_message, MAX_LENGHT, 0);

                    close(socket_fd);

                    printf("\033[?1049l"); // EXIT_ALT_SCREEN
                    fflush(stdout);

                    printf("%s", start_message);
                    
                    exit(0);
                    break;

                default:
                    recv_string(socket_fd, start_message, MAX_LENGHT, 0);
                    break;
        }

    }


}

void handle_login(int socket_fd){

    char message[MAX_LENGHT];
    char username[64], password[64];

    //Receive username request
    recv_string(socket_fd, message, MAX_LENGHT, 0);

    //CLEAR_SCREEN and CURSOR_HOME to put the cursor at the top left corner of the screen
    printf("\033[2J\033[H");

    printf("%s", message);

    scanf("%63s", username);

    //Cleans the buffer
    int c; 
    while ((c = getchar()) != '\n' && c != EOF);

    //Send username
    send_all(socket_fd, username, strlen(username) + 1);

    //Receive password request
    recv_string(socket_fd, message, MAX_LENGHT, 0);

    printf("%s", message);

    scanf("%63s", password);

    //Cleans the buffer 
    while ((c = getchar()) != '\n' && c != EOF);

    //Send password
    send_all(socket_fd, password, strlen(password) + 1);

    uint32_t status_received, status_code = 1;

    ssize_t r =recv_all(socket_fd, &status_received, sizeof(status_received), 0);

    if (r == sizeof(status_received))
        status_code = ntohl(status_received);


    //Receive login result message
    recv_string(socket_fd, message, MAX_LENGHT, 0);

    if(status_code == 0){

        //CLEAR_SCREEN and CURSOR_HOME to put the cursor at the top left corner of the screen
        printf("\033[2J\033[H");

        printf("%s", message);
        sleep(1);
        handle_session(socket_fd);

    }else{

        int done = 0;

        while (done == 0){

            //CLEAR_SCREEN and CURSOR_HOME to put the cursor at the top left corner of the screen
            printf("\033[2J\033[H");

            printf("%s", message);

            uint32_t option;
            uint32_t option_to_send;

            if (scanf("%u", &option) != 1) {
                option = UINT_MAX;     //Garbage value to send and so receive "Option not valid"
            }

            //Cleans the buffer
            int c;
            while ((c = getchar()) != '\n' && c != EOF);


            option_to_send = htonl(option);

            send_all(socket_fd, &option_to_send, sizeof(option_to_send));

            switch (option){
                    case 1:
                        done = 1;
                        handle_login(socket_fd);
                        return;
                    case 2:
                        done = 1;
                        handle_registration(socket_fd);
                        return;
                    case 3:
                        done = 1;

                        recv_string(socket_fd, message, MAX_LENGHT, 0);

                        close(socket_fd);

                        printf("\033[?1049l"); // EXIT_ALT_SCREEN
                        fflush(stdout);

                        printf("%s", message);
                        
                        exit(0);
                        break;

                    default:
                        
                        recv_string(socket_fd, message, MAX_LENGHT, 0);
                        break;
            }

        }


    }

}

void handle_registration(int socket_fd){

    char message[MAX_LENGHT];
    char username[64], password[64];

    //Receive username request
    recv_string(socket_fd, message, MAX_LENGHT, 0);

    //CLEAR_SCREEN and CURSOR_HOME to put the cursor at the top left corner of the screen
    printf("\033[2J\033[H");

    printf("%s", message);

    scanf("%63s", username);

    //Cleans the buffer
    int c; 
    while ((c = getchar()) != '\n' && c != EOF);

    //Send username
    send_all(socket_fd, username, strlen(username) + 1);

    //Receive password request
    recv_string(socket_fd, message, MAX_LENGHT, 0);

    printf("%s", message);

    scanf("%63s", password);

    //Cleans the buffer
    while ((c = getchar()) != '\n' && c != EOF);

    //Send password
    send_all(socket_fd, password, strlen(password) + 1);

    uint32_t status_received, status_code = 1;

    ssize_t r =recv_all(socket_fd, &status_received, sizeof(status_received), 0);

    if (r == sizeof(status_received))
        status_code = ntohl(status_received);


    //Receive registration result message
    recv_string(socket_fd, message, MAX_LENGHT, 0);

    if(status_code == 0){

        //CLEAR_SCREEN and CURSOR_HOME to put the cursor at the top left corner of the screen
        printf("\033[2J\033[H");

        printf("%s", message);
        sleep(1);
        handle_session(socket_fd);

    }else{

        int done = 0;

        while (done == 0){

            //CLEAR_SCREEN and CURSOR_HOME to put the cursor at the top left corner of the screen
            printf("\033[2J\033[H");

            printf("%s", message);

            uint32_t option;
            uint32_t option_to_send;

            if (scanf("%u", &option) != 1) {
                option = UINT_MAX;     //Garbage value to send and so receive "Option not valid"
            }

            //Cleans the buffer
            int c;
            while ((c = getchar()) != '\n' && c != EOF);


            option_to_send = htonl(option);

            send_all(socket_fd, &option_to_send, sizeof(option_to_send));

            switch (option){
                    case 1:
                        done = 1;
                        handle_registration(socket_fd);
                        return;
                    case 2:
                        done = 1;
                        handle_login(socket_fd);
                        return;
                    case 3:
                        done = 1;

                        recv_string(socket_fd, message, MAX_LENGHT, 0);

                        close(socket_fd);

                        printf("\033[?1049l"); // EXIT_ALT_SCREEN
                        fflush(stdout);

                        printf("%s", message);
                        
                        exit(0);
                        break;

                    default:
                        
                        recv_string(socket_fd, message, MAX_LENGHT, 0);
                        break;
            }

        }


    }


}

void handle_session(int socket_fd){

    char welcome_message_and_menu[MAX_LENGHT];

    recv_string(socket_fd, welcome_message_and_menu, MAX_LENGHT, 0);

    int done = 0;

    while (done == 0){

        //CLEAR_SCREEN and CURSOR_HOME to put the cursor at the top left corner of the screen
        printf("\033[2J\033[H");

        printf("%s", welcome_message_and_menu);

        uint32_t option;
        uint32_t option_to_send;

        if (scanf("%u", &option) != 1) {
            option = UINT_MAX;     //Garbage value to send and so receive "Option not valid"
        }

        //Cleans the buffer
        int c;
        while ((c = getchar()) != '\n' && c != EOF);


        option_to_send = htonl(option);


        send_all(socket_fd, &option_to_send, sizeof(option_to_send));

        switch (option){
                case 1:
                    done = 1;
                    lobby_creation(socket_fd);
                    return;
                case 2:
                    done = 1;
                    join_lobby(socket_fd);
                    return;
                case 3:
                    done = 1;

                    recv_string(socket_fd, welcome_message_and_menu, MAX_LENGHT, 0);

                    close(socket_fd);

                    printf("\033[?1049l"); // EXIT_ALT_SCREEN
                    fflush(stdout);

                    printf("%s", welcome_message_and_menu);
                    
                    exit(0);
                    break;

                default:
                    recv_string(socket_fd, welcome_message_and_menu, MAX_LENGHT, 0);
                    break;
        }

    }
    

}

void lobby_creation(int socket_fd){

    char size_request[MAX_LENGHT];

    recv_string(socket_fd, size_request, MAX_LENGHT, 0);

    int done = 0;

    while (done == 0){

        //CLEAR_SCREEN and CURSOR_HOME to put the cursor at the top left corner of the screen
        printf("\033[2J\033[H");

        printf("%s", size_request);

        uint32_t option;
        uint32_t option_to_send;

        if (scanf("%u", &option) != 1) {
            option = UINT_MAX;     //Garbage value to send and so receive "Option not valid"
        }

        //Cleans the buffer
        int c;
        while ((c = getchar()) != '\n' && c != EOF);


        option_to_send = htonl(option);


        send_all(socket_fd, &option_to_send, sizeof(option_to_send));

        if(option == 0 || option > 3)
            recv_string(socket_fd, size_request, MAX_LENGHT, 0);
        else
            done = 1;

    }

    handle_being_in_lobby(socket_fd);

}

void join_lobby(int socket_fd){

    char message_with_matches_info[MAX_LENGHT];

    recv_string(socket_fd, message_with_matches_info, MAX_LENGHT, 0);

    uint32_t status_received, status_code = 1;

    ssize_t r =recv_all(socket_fd, &status_received, sizeof(status_received), 0);

    if (r == sizeof(status_received))
        status_code = ntohl(status_received);


    if(status_code == 1){

        //CLEAR_SCREEN and CURSOR_HOME to put the cursor at the top left corner of the screen
        printf("\033[2J\033[H");

        printf("%s", message_with_matches_info);

        sleep(1);

        handle_session(socket_fd);
        return;

    }

    int done = 0;

    while (done == 0){

        //CLEAR_SCREEN and CURSOR_HOME to put the cursor at the top left corner of the screen
        printf("\033[2J\033[H");

        printf("%s", message_with_matches_info);

        uint32_t option;
        uint32_t option_to_send;

        if (scanf("%u", &option) != 1) {
            option = UINT_MAX;     //Garbage value to send and so receive "Option not valid"
        }

        //Cleans the buffer
        int c;
        while ((c = getchar()) != '\n' && c != EOF);

        option_to_send = htonl(option);

        send_all(socket_fd, &option_to_send, sizeof(option_to_send));

        if(option == 0){

            done = 1;

            //CLEAR_SCREEN and CURSOR_HOME to put the cursor at the top left corner of the screen
            printf("\033[2J\033[H");

            handle_session(socket_fd);
            return;
            
        }

        r = recv_all(socket_fd, &status_received, sizeof(status_received), 0);

        if (r == sizeof(status_received))
            status_code = ntohl(status_received);

        if(status_code == 0){
            done = 1;
            handle_being_in_lobby(socket_fd);
            return;
        }

        recv_string(socket_fd, message_with_matches_info, MAX_LENGHT, 0);

        r = recv_all(socket_fd, &status_received, sizeof(status_received), 0);

        if (r == sizeof(status_received))
            status_code = ntohl(status_received);


        if(status_code == 1){

            done = 1;

            //CLEAR_SCREEN and CURSOR_HOME to put the cursor at the top left corner of the screen
            printf("\033[2J\033[H");

            printf("%s", message_with_matches_info);

            sleep(1);

            handle_session(socket_fd);
            return;

        }

    }


}


void handle_being_in_lobby(int socket_fd){

    int stdin_open = 1;
    fd_set rset;
    
    char recvline[MAX_LENGHT];

    uint32_t status_received, status_code = 1;

    uint32_t option;
    uint32_t option_to_send;
	
    while (1) {
    	
    	FD_ZERO(&rset);
    	
    	if (stdin_open) 
			FD_SET(STDIN_FILENO, &rset);
		
        FD_SET(socket_fd, &rset);
        
        int maxfd = (STDIN_FILENO > socket_fd ? STDIN_FILENO : socket_fd) + 1;
        
        int nready = select(maxfd, &rset, NULL, NULL, NULL);

        if (nready < 0) {

            if (errno == EINTR) 
                continue;

            perror("select");
            close(socket_fd);
            exit(1);
            break;

        }
        
        //Socket ready: read from server
        if (FD_ISSET(socket_fd, &rset)) {

            ssize_t r = recv_all(socket_fd, &status_received, sizeof(status_received), 0);

            if (r == sizeof(status_received)){
                status_code = ntohl(status_received);
            }else{
                status_code = 1;    //if an error occured, makes sure that at least does't have "host privileges"
            }

            if(status_code == 2){
                handle_being_in_match(socket_fd);
                //no return, after match ending, back in this function e continuing the loop
                sleep(1);   //To be sure server has sent the new code and message
                continue;
            }

            r = recv_string(socket_fd, recvline, sizeof(recvline), 0);
            
            if (r == 0) {                 //Server closed
                if (stdin_open)
                    fprintf(stderr, "Server terminated prematurely\n");
                
                close(socket_fd);
                exit(1);
                break;
            }

            //CLEAR_SCREEN and CURSOR_HOME to put the cursor at the top left corner of the screen
            printf("\033[2J\033[H");

            printf("%s", recvline);

        }

        // stdin pronto: leggi e invia al server
        if (stdin_open && FD_ISSET(STDIN_FILENO, &rset)) {

            char input_buffer[64];

            if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL) {
                //EOF on stdin
                stdin_open = 0;
                option = UINT_MAX;     //Garbage value to send and so receive "Option not valid"

                if (shutdown(socket_fd, SHUT_WR) < 0) { 
                    perror("shutdown");

                    printf("\033[?1049l"); // EXIT_ALT_SCREEN
                    fflush(stdout);

                    exit(1);
                    break; 
                }
                
            }else {
                //Tries to extract a uint32_t from the line read
                if (sscanf(input_buffer, "%u", &option) != 1) {

                    option = UINT_MAX;      //Garbage value to send and so receive "Option not valid"

                }
            }
    
            option_to_send = htonl(option);

            //No need to clear the buffer since used fgets

            send_all(socket_fd, &option_to_send, sizeof(option_to_send));

            if(status_code == 0){
                
                switch (option){
                case 1:
                    handle_being_in_match(socket_fd);
                    break;
                case 2:
                    handle_change_map_size(socket_fd);
                    break;
                case 3:
                    handle_session(socket_fd);
                    break;
                case 4:
                    close(socket_fd);

                    printf("\033[?1049l"); // EXIT_ALT_SCREEN
                    fflush(stdout);

                    printf("%s", "\nArrivederci!\n");
                    
                    exit(0);
                    break;
                default:
                    //Doesn't do anything, the loop will read the message of option not valid in the next iteration
                    break;
                }

            }else if(status_code == 1){
                
                switch (option){
                case 1:
                    handle_session(socket_fd);
                    break;
                case 2:
                    close(socket_fd);

                    printf("\033[?1049l"); // EXIT_ALT_SCREEN
                    fflush(stdout);

                    printf("%s", "\nArrivederci!\n");
                    
                    exit(0);
                    break;
                default:
                    //Doesn't do anything, the loop will read the message of option not valid in the next iteration
                    break;
                }


            }

        }
        
    }


}

void handle_change_map_size(int socket_fd){

    char size_request[MAX_LENGHT];

    recv_string(socket_fd, size_request, MAX_LENGHT, 0);

    int done = 0;

    while (done == 0){

        //CLEAR_SCREEN and CURSOR_HOME to put the cursor at the top left corner of the screen
        printf("\033[2J\033[H");

        printf("%s", size_request);

        uint32_t option;
        uint32_t option_to_send;

        if (scanf("%u", &option) != 1) {
            option = UINT_MAX;     //Garbage value to send and so receive "Option not valid"
        }

        //Cleans the buffer
        int c;
        while ((c = getchar()) != '\n' && c != EOF);


        option_to_send = htonl(option);


        send_all(socket_fd, &option_to_send, sizeof(option_to_send));

        if(option == 0 || option > 3)
            recv_string(socket_fd, size_request, MAX_LENGHT, 0);
        else
            done = 1;

    }

}


void handle_being_in_match(int socket_fd){

    enable_terminal_game_mode();

    uint32_t size_received, size = 0;

    //First, reads map's size from the server
    ssize_t r =recv_all_in_match(socket_fd, &size_received, sizeof(size_received), 0);

    if (r == sizeof(size_received))
        size = ntohl(size_received);


    //Allocating the map
    char** map = malloc(size*sizeof(char*));

    if (!map) {

        perror("malloc");
        close(socket_fd);

        printf("\033[?1049l"); // EXIT_ALT_SCREEN
        fflush(stdout);

        printf("%s", "\nErrore fatale\n");
        
        exit(0);

    }

    int i, j;
    for(i=0; i < size; i++){

        map[i] = malloc(size*sizeof(char));

        if (!(map[i])) {

            perror("malloc");
            close(socket_fd);

            printf("\033[?1049l"); // EXIT_ALT_SCREEN
            fflush(stdout);

            printf("%s", "\nErrore fatale\n");
            
            exit(0);

        }

    }

    //Initialize map all empty ('e')
    for(i=0; i < size; i++)
        for(j=0; j < size; j++)
            map[i][j] = 'e';

    int stdin_open = 1;
    fd_set rset;

    uint32_t status_received, status_code = 1;
	
    while (1) {
    	
    	FD_ZERO(&rset);
    	
    	if (stdin_open) 
			FD_SET(STDIN_FILENO, &rset);
		
        FD_SET(socket_fd, &rset);
        
        int maxfd = (STDIN_FILENO > socket_fd ? STDIN_FILENO : socket_fd) + 1;
        
        int nready = select(maxfd, &rset, NULL, NULL, NULL);

        if (nready < 0) {

            if (errno == EINTR) 
                continue;

            perror("select");
            close(socket_fd);
            exit(1);
            break;

        }
        
        //Socket ready: read from server
        if (FD_ISSET(socket_fd, &rset)) {

            ssize_t r = recv_all_in_match(socket_fd, &status_received, sizeof(status_received), 0);

            if (r == sizeof(status_received)){
                status_code = ntohl(status_received);
            }else{
                status_code = UINT_MAX;    //if an error occured, garbage value
            }
            
            switch (status_code){
                case 2:
                    //Deallocating the map
                    for(i = 0; i < size; i++)
                            free(map[i]);
                    
                    free(map);

                    handle_match_ending(socket_fd);
                    return;
                case 0:
                    handle_local_info(socket_fd, map, size);
                    break;
                case 1:
                    handle_global_info(socket_fd, map, size);
                    break;
                default:
                    //Error occured, do nothing
                    break;
            }

        }

        // stdin pronto: leggi e invia al server
        if (stdin_open && FD_ISSET(STDIN_FILENO, &rset)) {

            char option;

            
            int nread = read(STDIN_FILENO, &option, 1);     //Reads 1 char (1 byte)

            if (nread <= 0) {
                //EOF on stdin
                stdin_open = 0;
                option = 'l';    // Garbage value

                if (shutdown(socket_fd, SHUT_WR) < 0) { 
                    perror("shutdown"); 
                    disable_terminal_game_mode();     //Disables game mode in terminal

                    printf("\033[?1049l"); // EXIT_ALT_SCREEN
                    fflush(stdout);

                    exit(1);
                    break; 
                }

            }

            //Sends the move to the server
            send_all_in_match(socket_fd, &option, sizeof(option));

        }
        
    }

    
}


void enable_terminal_game_mode() {

    tcgetattr(STDIN_FILENO, &orig_termios); //Saves previous attributes
    struct termios raw = orig_termios;
    
    // Disabilits ECHO (doesn't print pressed keys) and ICANON (to have instant reading, without having to wait to press enter)
    raw.c_lflag &= ~(ECHO | ICANON); 
    
    //Sets new attributes
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

}

void disable_terminal_game_mode() {

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);  //Sets previous attributes

    tcflush(STDIN_FILENO, TCIFLUSH);

}


void print_map(uint32_t players_positions[MAX_PLAYERS_MATCH][2], uint32_t my_id, char** map, uint32_t size){

    int i, j, p;

    //CLEAR_SCREEN and CURSOR_HOME to put the cursor at the top left corner of the screen
    printf("\033[2J\033[H");

    //Print upper border
    printf("  +");

    for(i=0; i < size; i++) 
        printf("--");

    printf("+\n");
    

    for (i = 0; i < size; i++) {
        printf("  |"); //Print left border
        
        for (j = 0; j < size; j++) {
            
            //Check players positions
            int player_on_cell = -1;
            for (p = 0; p < MAX_PLAYERS_MATCH; p++) {
                if (players_positions[p][0] == i && players_positions[p][1] == j) {
                    player_on_cell = p;
                    break; //Player founded
                }
            }

            //If there is a player, print its symbol and its color
            if (player_on_cell != -1) {

                if (player_on_cell == my_id) {

                    //This player
                    printf("%s%s★ %s", FG_WHITE_BOLD, bg_colors[player_on_cell], RESET);

                } else {

                    //Enemies
                    printf("%s%s♦ %s", FG_BLACK_BOLD, bg_colors[player_on_cell], RESET);

                }
                continue; //Go to next cell
            }

            //When there is no player
            switch (map[i][j]) {
                case 'e': 
                    printf("%s  %s", BG_BLACK, RESET); 
                    break;
                case 'w': 
                    printf("%s  %s", BG_WHITE, RESET); 
                    break;
                case 'b': 
                    printf("%s  %s", BG_BLUE, RESET); 
                    break;
                case 'r': 
                    printf("%s  %s", BG_RED, RESET); 
                    break;
                case 'g': 
                    printf("%s  %s", BG_GREEN, RESET); 
                    break;
                case 'y': 
                    printf("%s  %s", BG_YELLOW, RESET); 
                    break;
                default:  
                    printf("  "); 
                    break;
            }
        }
        printf("|\n"); //Print right border
    }


    //Print lower border
    printf("  +");

    for(i=0; i < size; i++) 
        printf("--");

    printf("+\n");


}

void handle_local_info(int socket_fd, char** map, uint32_t size){

    Message_with_local_information message_with_local_information;

    recv_all_in_match(socket_fd, &message_with_local_information, sizeof(message_with_local_information), 0);

    uint32_t my_id = ntohl(message_with_local_information.my_id);

    //Converts back all the coordinates received
    int p;
    for (p = 0; p < MAX_PLAYERS_MATCH; p++) {
        message_with_local_information.players_position[p][0] = ntohl(message_with_local_information.players_position[p][0]);
        message_with_local_information.players_position[p][1] = ntohl(message_with_local_information.players_position[p][1]);
    }

    //Coordinates of the player are the center of the local map received
    int x = message_with_local_information.players_position[my_id][0];
    int y = message_with_local_information.players_position[my_id][1];
    

    int i, j, z, k;
    z = 0;
    for(i= x - 2; i <= x + 2; i++, z ++){

        k = 0;
        for(j = y - 2; j <= y + 2; j++, k++ ){

                if( (i >= 0 && i < size)  && (j >= 0 && j < size) ){            //If i and j are "good" indexes

                    map[i][j] = message_with_local_information.local_map[z][k];

                }


        }
    }

    print_map(message_with_local_information.players_position, my_id, map, size);

    printf("%s\n", message_with_local_information.message);

}

void handle_global_info(int socket_fd, char** map, uint32_t size){

    Global_Info_Header global_Info_Header;

    recv_all_in_match(socket_fd, &global_Info_Header, sizeof(global_Info_Header), 0);

    uint32_t my_id = ntohl(global_Info_Header.my_id);

    //Converts back all the coordinates received
    int p;
    for (p = 0; p < MAX_PLAYERS_MATCH; p++) {
        global_Info_Header.players_position[p][0] = ntohl(global_Info_Header.players_position[p][0]);
        global_Info_Header.players_position[p][1] = ntohl(global_Info_Header.players_position[p][1]);
    }

    char* new_global_map = malloc(size*size*sizeof(char));

    if (!new_global_map) {

        perror("malloc");
        close(socket_fd);

        printf("\033[?1049l"); // EXIT_ALT_SCREEN
        fflush(stdout);

        printf("%s", "\nErrore fatale\n");
        
        exit(0);

    }

    recv_all_in_match(socket_fd, new_global_map, size*size*sizeof(char), 0);

    int i, j;
    for(i = 0; i < size; i++){
        for(j = 0; j < size; j++){

            if(map[i][j] != 'w')
                map[i][j] = new_global_map[ (i*size) + j ];


        }
    }

    free(new_global_map);

    print_map(global_Info_Header.players_position, my_id, map, size);

    printf("%s\n", "\nMuoversi con w, a, s, d");

}


void handle_match_ending(int socket_fd){

    char ending_message[MAX_LENGHT];

    //Receive ending message
    recv_string_in_match(socket_fd, ending_message, MAX_LENGHT, 0);

    printf("%s", ending_message);

    sleep(30);

    disable_terminal_game_mode();

}


