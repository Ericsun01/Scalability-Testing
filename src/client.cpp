#include <arpa/inet.h>
#include <assert.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sstream>
#include <mutex>
#include <thread>
#include <stdlib.h>
#include <time.h>
#include "ctpl_stl.h"


using namespace std;
int socket_fd;

    
void connect_server(const char *hostname, const char *port) {

    int status;
    struct addrinfo host_info;
    struct addrinfo *host_info_list;

    memset(&host_info, 0, sizeof(struct addrinfo));
    host_info.ai_family = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;

    status = getaddrinfo(hostname, port, &host_info, &host_info_list);
    if (status != 0) {
        std::cout << "cannot get address info for host" << std::endl;
        std::cout << "  (" << hostname << "," << port << ")" << std::endl;
        exit(EXIT_FAILURE);
    } // if

    socket_fd = socket(host_info_list->ai_family, host_info_list->ai_socktype,
                    host_info_list->ai_protocol);
    if (socket_fd == -1) {
        std::cout << "cannot create socket" << std::endl;
        std::cout << "  (" << hostname << "," << port << ")" << std::endl;
        exit(EXIT_FAILURE);
    } // if

    status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
        std::cout << "cannot connect to socket" << std::endl;
        std::cout << "  (" << hostname << "," << port << ")" << std::endl;
        exit(EXIT_FAILURE);
    } // if

    freeaddrinfo(host_info_list);
}

void send_msg(string msg) {
    
    send(socket_fd, msg.c_str(), strlen(msg.c_str()), 0);

}

void recv_response() {
    char buffer[128];
    int len = recv(socket_fd, buffer, sizeof(buffer), MSG_WAITALL);
    if (len == -1) {
        perror("recv");
    }
    buffer[len] = 0;
    // DEBUG
    cout << "Received back from server: " << buffer << endl;
}


string msg_generator(unsigned int delay, unsigned int bucket_to_add) {
    stringstream sbuffer;
    sbuffer << delay << "," << bucket_to_add << endl;
    string msg = sbuffer.str();
    msg += "\n";

    return msg;    
}

void run_client(int id, const char * hostname, string msg_to_send) {
    connect_server(hostname, "12345");
    send_msg(msg_to_send);
    recv_response();
    close(socket_fd);
}

  
int main(int argc, char **argv) {

    srand(time(NULL));
  
    ctpl::thread_pool p(10);
    while (true) {
      unsigned int delay = rand() % atoi(argv[2]) + 1;
      unsigned int bucket_to_add = rand() % atoi(argv[3]);
      string msg_to_send = msg_generator(delay, bucket_to_add);
      p.push(run_client, argv[1], msg_to_send);
      //run_client(argv[1], msg_to_send);
    }

    return EXIT_SUCCESS;
}
