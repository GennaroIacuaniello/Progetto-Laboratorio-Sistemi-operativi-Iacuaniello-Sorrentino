#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

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

per partita, lista informazione giocatore:

nodo: posizione_x, posizione_y, num celle colorate


mappe: 4 preset 16x16 scelti casualmente dal server (forse?)


*/

typedef struct User{

      unsigned char username[64];
      unsigned long hashed_password;

}User;

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

const unsigned int map_preset_1[16][16] = {
      {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
      {0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0},
      {0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0},
      {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0},
      {0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0},
      {0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 1, 1},
      {1, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0},
      {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0},
      {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0},
      {0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0},
      {0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0},
      {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0}
};

const unsigned int map_preset_2[16][16] = {
      {0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0},
      {0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0},
      {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0},
      {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0},
      {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0},
      {0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0},
      {0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
      {0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0},
      {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0},
      {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0},
      {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0},
      {0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0},
      {0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0}
};

const unsigned int map_preset_3[16][16] = {
      {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0},
      {1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0},
      {0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0},
      {0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0},
      {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0},
      {0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 1, 0},
      {0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0},
      {0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0},
      {0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0},
      {0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0},
      {0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0},
      {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0},
      {0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0},
      {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

const unsigned int map_preset_4[16][16] = {
      {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0},
      {0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 0},
      {0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0},
      {0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0},
      {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0},
      {0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0},
      {0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1},
      {0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1},
      {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
      {0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1},
      {0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1},
      {0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1},
      {0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1},
      {0, 0, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0}
};


unsigned int login(unsigned char* username, unsigned char* password);
unsigned int registration(unsigned char* username, unsigned char* password);
unsigned long hash(unsigned char *str);

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


int main(int argc, char *argv[]){

      srand(time(NULL));


      int res = login("sdf", "dsfsd");
      
      printf("%d\n", res);
      read_users();



      return 0;
}











unsigned int login(unsigned char* username, unsigned char* password){

      FILE* file_users = fopen("users.dat", "rb");
      unsigned long hashed_password = hash(password);

      if(file_users != NULL){
            
            User current_user;

            while(fread(&current_user, sizeof(User), 1, file_users) == 1){

                  if( ( strcmp(username, current_user.username) == 0 ) &&
                        ( hashed_password == current_user.hashed_password ) ){
                              
                              fclose(file_users);
                              return 0;

                        }
                        

            }
            fclose(file_users);
            return 1;

      }else{
            perror("fopen");
            return 2;
      }

}

unsigned int registration(unsigned char* username, unsigned char* password){

      FILE* file_users = fopen("users.dat", "rb+");

      if(file_users != NULL){
            
            User current_user;

            while(fread(&current_user, sizeof(User), 1, file_users) == 1){

                  if( strcmp(username, current_user.username) == 0 ){
                              
                              fclose(file_users);
                              return 1;

                        }
                        

            }
            
            strcpy(current_user.username, username);
            unsigned long hashed_password = hash(password);
            current_user.hashed_password = hashed_password;

            fwrite(&current_user, sizeof(User), 1, file_users);

            fclose(file_users);
            return 0;

      }else{
            perror("fopen");
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

