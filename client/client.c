//Compile: gcc client.c authentication/authentication.c network/network.c session/session.c match/match.c -I./authentication -I./network -I./session -I./match -o client.out
//Run: ./client Server-Name/IP PORT

#include "authentication/authentication.h"
#include "network/network.h"
#include "session/session.h"
#include "match/match.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>    //To handle "moving in the map" during the game
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <limits.h>


void check_terminal_size();         //Initial function to warn the user in case of too litle terminal

void handle_starting_interaction(int socket_fd);

void handle_sigint(int sig) {
    
    printf("\033[?1049l");  //EXIT_ALT_SCREEN
    
    printf("\033[?25h");    //SHOW_CURSOR

    fflush(stdout);
    exit(0);

}

int main(int argc, char* argv[]) {

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, handle_sigint);

    init_terminal();

    int socket_fd;
    struct sockaddr_in server_addr;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <server_ip/name> <port>\n", argv[0]);
        exit(1);
    }

    //Initial terminal size check
    check_terminal_size();

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    //argv[1] is the host ("127.0.0.1" or "server-cont"), argv[2] is the port ("8080")
    int err = getaddrinfo(argv[1], argv[2], &hints, &res);
    if (err != 0) {
        fprintf(stderr, "Errore getaddrinfo: %s\n", gai_strerror(err));
        exit(1);
    }

    socket_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (socket_fd < 0) {
        perror("socket");
        freeaddrinfo(res);
        exit(1);
    }

    if (connect(socket_fd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("connect");
        freeaddrinfo(res);
        exit(1);
    }

    freeaddrinfo(res);

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
