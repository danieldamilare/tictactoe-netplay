#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string>
#include <netinet/in.h>
#include <iostream>
#include <unistd.h>
#include "gamelogic.h"
#include "log.h"


#define PORT "8080"
void * get_sock_in(struct sockaddr * addr){
    if (addr->sa_family == AF_INET){
        return &(((struct sockaddr_in *) addr)->sin_addr);
    }
    return &(((struct sockaddr_in6 *)addr)->sin6_addr); 
}

class TictactoeServer{
public:

    TictactoeServer(std::string serv):
        servfd{-1}, clifd1{-1}, clifd2{-1}, service{serv}{}

    int accept_connection(struct sockaddr_storage& addr, socklen_t& addrsiz){
        int sockfd{};
        while((sockfd = accept(servfd, 
                        (struct sockaddr *)&addr, &addrsiz)) == -1){
#ifdef LOGSOCK
            perror("Error accepting user");
#endif
            if (errno == EINTR)
                continue;
            perror("Error accepting connection");
            return sockfd;
            
        }
        return sockfd;
    }
    
    void run(){
        struct addrinfo * servinfo{};
        servinfo= get_addr();
        if (servinfo == nullptr){
            return;
        }

        if ((servfd = initialize_socket(servinfo)) == -1){
            freeaddrinfo(servinfo);
            servinfo = nullptr;
            return;
        }
        freeaddrinfo(servinfo);
        servinfo = nullptr;

        socklen_t adsiz = sizeof(first_player);
        LOG("Waiting for user to connect");

        clifd1 = accept_connection(first_player, adsiz);
        if (clifd1 < 0) return;
        inet_ntop(first_player.ss_family, 
                  get_sock_in((struct sockaddr *)&first_player),
                  first_buf, sizeof first_buf
        );
        LOG("{} connected", first_buf);
        HumanPlayer player1{'X', clifd1};
        send_message(player1, "mesg:Connected. Waiting for 2nd player...");

        clifd2 = accept_connection(second_player, adsiz);
        if (clifd2 < 0) return;
        inet_ntop(second_player.ss_family, 
                  get_sock_in((struct sockaddr *) &second_player),
                  second_buf, sizeof second_buf);

        LOG("{} connected", second_buf);
        HumanPlayer player2{'O', clifd2};

        Game game{player1, player2};
        game.play_game();
        std::cout << "Game finished. Shutting down server";
        close(servfd);
    }

private:
    int servfd{}, clifd1{}, clifd2{};
    std::string service;
    static constexpr int BACKLOG =  10;
    struct sockaddr_storage first_player{}, second_player{};
    char first_buf[INET6_ADDRSTRLEN]{}, second_buf[INET6_ADDRSTRLEN]{};

    struct addrinfo * get_addr(){
        int status{};
        struct addrinfo hints{};
        struct addrinfo *servinfo{};
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;

        if ((status = getaddrinfo(nullptr, service.c_str(), &hints, &servinfo)) == -1){
            std::cerr << "Error getting port " << service << ": " << 
                gai_strerror(status) << std::endl;
            return nullptr;
        }
        return servinfo;
    }


    static int initialize_socket(struct addrinfo * servinfo){
        struct addrinfo * ptr{};
        int servfd{};
        int yes = 1;

        for (ptr = servinfo; ptr != nullptr; ptr = ptr->ai_next){
            if ((servfd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) == -1)
                continue;

            if (setsockopt(servfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1){
                perror("setsockopt");
                close(servfd);
                continue;
            }

            if (bind(servfd, ptr->ai_addr, ptr->ai_addrlen) == -1){
                perror("bind");
                close(servfd);
                continue;
            }
            break;
        }

        if (ptr == nullptr){
            std::cerr << "Failed to bind any address\n";
            return -1;
        }
        LOG("Binded to socket");
        if (listen(servfd, BACKLOG) == -1){
            close(servfd);
            perror("Error listening to socket");
            return -1;
        }
        return servfd;
    }
};


int main(){
    TictactoeServer server(std::string{PORT});
    server.run();
    return 0;
}
