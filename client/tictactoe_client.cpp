#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <array>
#include <string>
#include <algorithm>
#include "../log.h"

struct Client{
    Client(int fd): fd{fd}, move{}{
        Pbuf.reserve(1024);
    }

    ~Client(){
            if(fd >= 0) close(fd);
    }

    int write_data(const char buf[], size_t buflen) const{
        LOG("Writing data for player: {}: {}", move, buf);
        size_t rem_bytes{buflen};
        int cur{};
        const char * str = buf;

        while (rem_bytes > 0){
            cur = send(fd, str, rem_bytes, 0);
            if (cur == -1) { return cur; }
            rem_bytes -= cur;
            str = str + cur;
        }
        return static_cast<int>(buflen - rem_bytes);
    }


        // uses buflen -1, store '\0' in the last byte
    int read_data(char buf[], size_t buflen){ /* buflenn is always the uppe
                                                         rbound amount to read*/
        while(1){
            size_t pos = Pbuf.find('\n');
            if (pos != std::string::npos){

                int len = std::min(buflen-1, pos+1);
                std::copy_n(Pbuf.begin(), len, buf);
                buf[len] = 0;
                Pbuf.erase(0, pos+1);

                LOG("Receiving data for player{}: {}", move,  buf);
                return len;
            }
                char rb[512];
                int result = recv(fd, rb, 512, 0);
                if (result <= 0){
                    if(result < 0) perror("recv");
                    LOG("Player disconnected or error reading input");
                    return result;
                }
                Pbuf.append(rb, result);
        }
    }

    int  process_game(std::string& res){ /* return -1 if game should end, 1 if a move has just been made by player */
        LOG("In process game res: {}", res);
        if (res.compare(0, 5, "mesg:") == 0){
            std::cout << res.substr(5);
        } 
        else if (res.compare(0, 6, "ready:") == 0){
            LOG("In ready");
            move = res.at(6);
            std::cout<< "Game Ready\n You are " << move << "\n";
        } 
        else if (res.compare(0, 5, "play:") == 0){
            int pos = res.at(7) - '0';
            LOG("pos: {}, mov: {}, player move: {}", pos, res.at(5), move) ;
            board[pos] = res.at(5);

            if (res.at(5) == move){
                std::cout << "You played at " << pos <<"\n";
            } else std::cout << "Opponent played at " << pos << "\n";
            LOG("Player move: {}", move);
            print_board();
        } 
        else if (res.compare(0, 4, "err:") ==0){
            std::cout << "\033[1;31m" << res.substr(4) << "\033[0m\n";
        } 

        else if (res.compare("move\n") == 0){
            std::cout << "Enter a move (0 - 8). Type quit to quit\n";
            std::string line;
            std::getline(std::cin, line);
            line += '\n';
            if (write_data(line.c_str(), line.length()) == -1){
                std::cerr << "Error communicating to server\n";
                return -1;
            }
            return 1;
        }

        else if (res.compare(0, 5, "over:") == 0){
            LOG("In over*");
            std::string s = res.substr(5);
            
            if (s == "disconnect\n"){
                std::cout << "Game Ended\n... Quiting\n";
                return -1;
            } else if (s =="won\n"){
                std::cout << "\033[1mYou won!!!\033[0m\nQuitting the Game\n";
                return -1;
            } else if (s =="lost\n"){
                std::cout << "\033[1mYou Lost!!!\033[0m\nQuitting the Game...\n";
                return -1;
            } else if (s == "tie\n"){
                std::cout << "\033[1mGame ended in a tie!!!\033[0m\nQuitting the Game...\n";
                return -1;
            }
        }
        return 0;
    }

    void play_game(){
        board.fill('\0');
        while(true){
            int st = read_data(buf, BUFSIZ);
            LOG("Just reading from input. status: {}", st);
            if(st <= 0){
                break;
            }
            std::string res{buf};
            LOG("Client recieved: {}", buf);
            std::cout << "CAlling PRocess game\n";

            int status = process_game(res);
            LOG("Recieved: status: {}", status);

            if(status == -1) { break; }
            else if (status == 1){
                std::cout << "Waiting for second player...\n";
            }
        }
    }
    void print_board(){
        std::cout << "+---+---+---+\n";
        for (int i = 0; i < 9; i++){
            std::cout << "| " << (board[i] == 0? ' ': board[i]) << " " ;
            if ((i+1) % 3 == 0){
                std::cout  << "|\n";
                std::cout << "+---+---+---+\n";
            }
        }
    }

    private:
        int fd{};
        std::string Pbuf{};
        std::array<char, 9> board{};
        char buf[BUFSIZ]{};
        char move{};
        /* int MAX_SIZE{1024}; */
};

static std::string port;
constexpr const char * DEFAULT_PORT{"8080"};
static std::string host;
int sockfd{-1};




bool connect_server(){
    int status{};
        struct addrinfo hints{}, *servinfo{}, *p{};
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        if ((status = getaddrinfo(host.c_str(), port.c_str(), &hints, &servinfo)) == -1){
            std::cerr << "Error resolving address: " << gai_strerror(status);
            return false;
        }

        for (p = servinfo; p != nullptr; p = p->ai_next){
            if ((sockfd =socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
#ifdef LOGSOCK
                perror("socket");
#endif
                continue;
            }

            if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1){
#ifdef LOGSOCK
                perror("connect");
#endif
                close(sockfd);
                sockfd = -1;
                continue;
            }
            break;
        }

        if (p == NULL){
            freeaddrinfo(servinfo);
            std::cerr << "Error: Can't connect to address\n";
            return false;
        }
        freeaddrinfo(servinfo);
        return true;
}


int main(int argc, char ** argv){
    if (argc < 2 || argc > 3){
        std::cerr << "usage: " << argv[0] << " host port\n";
        exit(1);
    }
    host = argv[1];
    port = argc == 3? argv[2]: DEFAULT_PORT;
    #include <iostream>

    std::cout << R"(
        ,----,                                     ,----,                                ,----,
      ,/   .`|                                   ,/   .`|                              ,/   .`|  ,----..
    ,`   .'  :   ,---,  ,----..                ,`   .'  : ,---,         ,----..      ,`   .'  : /   /   \      ,---,.
  ;    ;     /,`--.' | /   /   \             ;    ;     /'  .' \       /   /   \   ;    ;     //   .     :   ,'  .' |
.'___,/    ,' |   :  :|   :     :    ,---,..'___,/    ,'/  ;    '.    |   :     :.'___,/    ,'.   /   ;.  \,---.'   |
|    :     |  :   |  '.   |  ;. /  ,'  .' ||    :     |:  :       \   .   |  ;. /|    :     |.   ;   /  ` ;|   |   .'
;    |.';  ;  |   :  |.   ; /--` ,---.'   ,;    |.';  ;:  |   /\   \  .   ; /--` ;    |.';  ;;   |  ; \ ; |:   :  |-,
`----'  |  |  '   '  ;;   | ;    |   |    |`----'  |  ||  :  ' ;.   : ;   | ;    `----'  |  ||   :  | ; | ':   |  ;/|
    '   :  ;  |   |  ||   : |    :   :  .'     '   :  ;|  |  ;/  \   \|   : |        '   :  ;.   |  ' ' ' :|   :   .'
    |   |  '  '   :  ;.   | '___ :   |.'       |   |  ''  :  | \  \ ,'.   | '___     |   |  ''   ;  \; /  ||   |  |-,
    '   :  |  |   |  ''   ; : .'|`---'         '   :  ||  |  '  '--'  '   ; : .'|    '   :  | \   \  ',  / '   :  ;/|
    ;   |.'   '   :  |'   | '/  :              ;   |.' |  :  :        '   | '/  :    ;   |.'   ;   :    /  |   |    \
    '---'     ;   |.' |   :    /               '---'   |  | ,'        |   :    /     '---'      \   \ .'   |   :   .'
              '---'    \   \ .'                        `--''           \   \ .'                  `---`     |   | ,'
                        `---`                                           `---`                              `----'
    )";

    std::cout << "Connecting to server...\n";
    if (!connect_server()){
        return 1;
    } 
    std::cout << "Connected\n";

    Client pl{sockfd};
    pl.play_game();
}
