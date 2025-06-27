#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string>
#include <netinet/in.h>
#include <iostream>
#include <unistd.h>


#define PORT "8080"

void * get_sock_in(struct sockaddr * addr){
    if (addr->sa_family == AF_INET)
        return &(((struct sockaddr_in *) addr)->sin_addr);
    else return &(((struct sockaddr_in6 *)addr)->sin6_addr);
}

class TictactoeServer{
    int servfd, clifd1, clifd2;
    char buffer[BUFSIZ];
    char * service;
    static constexpr int BACKLOG =  10;
    struct sockaddr_storage first_player{}, second_player{};

    char first_buf[INET6_ADDRSTRLEN], second_buf[INET6_ADDRSTRLEN];
    std::string buf;

    struct addrinfo * get_addr(){
        int status;
        struct addrinfo hints{}, *servinfo{};
        hints.ai_family = AF_UNSPEC;
        hints.ai_flags = AI_PASSIVE;
        if ((status = getaddrinfo(NULL, service, &hints, &servinfo)) == -1){
            std::cout << "Error getting port " << service << ": " << gai_strerror(status) << std::endl;
            return nullptr;
        }
        return servinfo;
    }
    

    TictactoeServer(char * serv): service{serv}{}
    void run_server(void){
        struct addrinfo * servinfo{}, *p{};
        servinfo= get_addr();
        if (servinfo == NULL){
            return;
        }

        if ((servfd = initialize_socket(servinfo)) == -1)
            return;
        freeaddrinfo(servinfo);
        socklen_t adsiz = sizeof(first_player);
#ifdef LOGSOCK
        std::cout <<  "Waiting for user to connect\n";
#endif
        while((clifd1 = accept(servfd, (struct sockaddr *)&first_player, &adsiz)) == -1){
#ifdef LOGSOCK
            perror("Error accepting user");
#endif
            continue;
        }

        inet_ntop(first_player.ss_family, 
                  get_sock_in((struct sockaddr *)&first_player),
                  first_buf, sizeof first_buf
        );

        std::cout << first_buf << " connected\n";
        
        while((clifd2 = accept(servfd, (struct sockaddr *)&second_player, &adsiz)) == -1){
#ifdef LOGSOCK
            perror("Error accepting user");
#endif
            continue;
        }

        inet_ntop(second_player.ss_family, 
                  get_sock_in((struct sockaddr *) &second_player),
                  second_buf, sizeof second_buf);

        std::cout << second_buf << "connected\n";
        close(servfd);
    }

    int initialize_socket(struct addrinfo * servinfo){
        struct addrinfo * p;
        int servfd{};
        for (p = servinfo; p != NULL; p = p->ai_next){
            if ((servfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
                continue;

            if (bind(servfd, p->ai_addr, p->ai_addrlen) == -1){
                close(servfd);
                continue;
            }
            break;
        }

        if (p == NULL){
            std::cerr << "Error connecting to socket\n";
            return -1;
        }
#ifdef LOGSOCK
        cout << "Binded to socket\n"
#endif
        if (listen(servfd, BACKLOG) == -1){
            close(servfd);
            perror("Error listening to socket");
            return -1;
        }
        return servfd;
    }
};


