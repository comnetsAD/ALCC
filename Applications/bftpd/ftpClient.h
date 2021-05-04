#ifndef CLIENT_H
#define CLIENT_H

void openTCP(struct sockaddr_in * server_addr, int * port, char * ip_addr, int * sock_fd);
int change_directory(char * current_directory, char * new_directory);
int list_client_files(char * current_directory, char * path);
void get_file(char * cur_dir, char * filename, struct sockaddr_in * server_addr, int * port, char * ip_addr, int * sock_fd);
void put_file(char * filename, struct sockaddr_in * server_addr, int * port, char * ip_addr, int * sock_fd, int src);
void parse_arg_to_buffer(char * command, char * params, int sock_fd, char * buffer);
int open_socket(struct sockaddr_in * myaddr, int * port, char * addr, int * sock);

#endif