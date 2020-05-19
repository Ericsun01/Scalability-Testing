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
#include <vector>
#include <sys/time.h>
#include <thread>
#include <queue>
#include <mutex>
#include <fstream>
#include <time.h>
#include "ctpl_stl.h"

using namespace std;

struct addrinfo host_info, *host_info_list;
int sockfd; // fd for socket
int status;
vector<unsigned int> bucket;
mutex m1;
int counter = 0;

    void thread_counter() {
        // debug
        cout << "counter thread starts" << endl;
        ofstream logfile("throughput.txt", ostream::out);
        while (true) {
            struct timeval start, end;
            double elapsed_seconds;
            gettimeofday(&start, NULL);
            do {
                gettimeofday(&end, NULL); 
                elapsed_seconds =
                (end.tv_sec + (end.tv_usec/1000000.0)) -
                (start.tv_sec + (start.tv_usec/1000000.0)); 
            } while (elapsed_seconds < 10);
            // when time reaches
            // logfile.clear();
            logfile << "Number of request handled: " << counter << endl;
            logfile.flush();
        }
        
    }

    void initialize(const char *_port) {  //  use getaddrinfo() to initialize
        memset(&host_info, 0, sizeof(host_info));
        host_info.ai_family = AF_UNSPEC;
        host_info.ai_socktype = SOCK_STREAM;
        host_info.ai_flags = AI_PASSIVE;

        status = getaddrinfo(NULL, _port, &host_info, &host_info_list);
        if (status != 0) {
            std::cerr << "Error: cannot get address info for host" << std::endl;
            exit(EXIT_FAILURE);
        }
    }


    void create_socket() {      // build socket
        sockfd = socket(host_info_list->ai_family, host_info_list->ai_socktype,
                        host_info_list->ai_protocol);
        if (sockfd == -1) {
            std::cout << "cannot create socket" << std::endl;
            exit(EXIT_FAILURE);
        } 

        int yes = 1;
        status = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        status = bind(sockfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
        if (status == -1) {
            std::cout << "cannot bind socket" << std::endl;
            exit(EXIT_FAILURE);
        } 

        status = listen(sockfd, 100);
        if (status == -1) {
            std::cout << "cannot listen on socket" << std::endl;
            exit(EXIT_FAILURE);
        } 

        freeaddrinfo(host_info_list);
    }

    void buildServer(const char *port) {  // begin the server, for master
        initialize(port);
        create_socket();
    }

    int accept_connection() {  //accept the new connection
        int newfd;
        struct sockaddr_storage socket_addr;
        socklen_t socket_addr_len = sizeof(socket_addr);
        newfd = accept(sockfd, (struct sockaddr *)&socket_addr, &socket_addr_len);
        if (newfd == -1) {
            std::cerr << "Error: cannot accept connection on socket" << std::endl;
            exit(EXIT_FAILURE);
        } // if

        return newfd;
    }

    pair<unsigned int, unsigned int> parse_recv_msg(string msg) {
        pair<unsigned int, unsigned int> info_pair;
        unsigned int delay, bucket_to_add;
        
        size_t pos1 = msg.find_first_of(",");
        if (pos1 != string::npos) {
            delay = stoul(msg.substr(0, pos1), &pos1, 10);
            info_pair.first = delay;
        }
        // parse out append value from message
        size_t pos2 = msg.find_first_of("\n");
        if (pos2 != string::npos) {
            bucket_to_add = stoul(msg.substr(pos1 + 1, pos2), &pos2, 10);
            info_pair.second = bucket_to_add;
        }

        return info_pair;
    }

    // handle request
    void handle_request(int client_fd) {
        // debug
        cout << "per-request" << endl;
        
        // auto th_id = this_thread::get_id();
        // m1.lock();
        // cout << "Thread ID: "<< th_id << endl;
	    // m1.unlock();
        char buffer[128];
        recv(client_fd, buffer, sizeof(buffer), 0);
        buffer[127] = 0;
        // DEBUG
        cout << "Server received: " << buffer;

        // parse out delay and bucket_to_add from message
        string msg(buffer);
        // cout << "Convert to string: " << msg << endl;
        pair<unsigned int, unsigned int> info_pair = parse_recv_msg(msg);
        unsigned int delay = info_pair.first;
        unsigned int num_to_add = info_pair.second;
        // DEBUG
        // cout << "delay is: " << delay << endl;
        // cout << "bucket_to_add is: " << num_to_add << endl; 

        // handle request
        // delay operation
        struct timeval start, check;
        double elapsed_seconds;
        gettimeofday(&start, NULL);
        do {
            gettimeofday(&check, NULL); elapsed_seconds =
            (check.tv_sec + (check.tv_usec/1000000.0)) - 
            (start.tv_sec + (start.tv_usec/1000000.0)); 
        } while (elapsed_seconds < delay);

        // add delay to bucket        
        m1.lock();
        bucket.at(num_to_add) = bucket.at(num_to_add) + delay;
        unsigned int updated_num = bucket[num_to_add];
        m1.unlock();
        // send response
        // construct response msg
        string response = to_string(updated_num) + "\n";
        // send back to client
        send(client_fd, response.c_str(), strlen(response.c_str()), 0);
        counter++;
        close(client_fd);
    }

    void handle_request1(int id, int client_fd) {
      
	    // cout << "Thread ID: "<< id << endl;
        char buffer[128];
        int len = recv(client_fd, buffer, sizeof(buffer), 0);
        if (len == -1) {
            perror("recv");
        }
        buffer[len] = 0;
        // DEBUG
        cout << "Server received: " << buffer;

        // parse out delay and bucket_to_add from message
        string msg(buffer);
        pair<unsigned int, unsigned int> info_pair = parse_recv_msg(msg);
        unsigned int delay = info_pair.first;
        unsigned int num_to_add = info_pair.second;
        // DEBUG
        cout << "delay is: " << delay << endl;
        cout << "bucket_to_add is: " << num_to_add << endl; 

        // handle request
        // delay operation
        struct timeval start, check;
        double elapsed_seconds;
        gettimeofday(&start, NULL);
        do {
            gettimeofday(&check, NULL); elapsed_seconds =
            (check.tv_sec + (check.tv_usec/1000000.0)) - 
            (start.tv_sec + (start.tv_usec/1000000.0)); 
        } while (elapsed_seconds < delay);

        // add delay to bucket
        bucket.at(num_to_add) = bucket.at(num_to_add) + delay;
        unsigned int updated_num = bucket[num_to_add];
        // send response
        // construct response msg
        string response = to_string(updated_num) + "\n";
	    cout << "response is " << response << endl;

        // send back to client
        send(client_fd, response.c_str(), strlen(response.c_str()), 0);
        counter++;
        close(client_fd);
    }


    int main(int argc, char **argv) {
        // create bucket
        unsigned int num_of_bucket = atoi(argv[1]);
        int method = atoi(argv[2]);
        bucket = vector<unsigned int>(num_of_bucket);

        buildServer("12345");

        // a thread for timing
        thread timing_thread(thread_counter);

        if (method == 0) { // if create thread per request
            while(true) {
                int client_fd = accept_connection();
                if (client_fd == -1){
                    perror("accept");
                    continue;
                }           
                thread t1(handle_request, client_fd);
                t1.detach();
                // t1.join();	
            }
        }
        else if (method == 1) { // if pre-create
            ctpl::thread_pool p(10);
            cout << "pool created " << endl;
            while(true) {
                int client_fd = accept_connection();
                // cout << "client_fd is " << client_fd << endl;
                p.push(handle_request1, client_fd);
            }    

        }
        else {
            cerr << "method must be 0 or 1!" << endl;
        }

        return EXIT_SUCCESS;

    }
