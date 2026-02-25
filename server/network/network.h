#ifndef NETWORK_H
#define NETWORK_H

#include <sys/types.h>

typedef struct User User;
typedef struct Match_list_node Match_list_node;

ssize_t send_all(int socket_for_thread, const void* buf, size_t n);
ssize_t recv_all(int socket_for_thread, void* buf, size_t n, int flags);
ssize_t recv_string(int socket_for_thread, char* buf, size_t max_len, int flags);

ssize_t send_all_in_match(int socket_for_thread, const void* buf, size_t n, Match_list_node* match_node, User* current_user, int id_in_match);
ssize_t recv_all_in_match(int socket_for_thread, void* buf, size_t n, int flags, Match_list_node* match_node, User* current_user, int id_in_match);

#endif