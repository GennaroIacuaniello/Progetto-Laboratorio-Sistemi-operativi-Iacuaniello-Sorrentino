#ifndef AUTHENTICATION_H
#define AUTHENTICATION_H

typedef struct User{

      char username[64];
      unsigned long hashed_password;

}User;

User* handle_login(int socket_for_thread);
User* handle_registration(int socket_for_thread);


#endif