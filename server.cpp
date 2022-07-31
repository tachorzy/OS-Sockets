// A simple server in the internet domain using TCP by Robert Ingalls

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <string>
#include <algorithm>
#include <iostream>
#include <vector>
#include <cmath>
#include <map>

//Fireman: waitpid() suspends execution of a calling process until a child specified by a pid argument has changed state.
void fireman(int) {
   while (waitpid(-1, NULL, WNOHANG) > 0) //waitpid() > 0 means its waiting for a child whose PID equals its PID, WNOHANG tells it to return immediately if no child has exited
      ;
}

std::string bin(int n, int b){ //converts an int (n) to a binary value in the size of b (number of bits)
    std::string s;
    for(int i = b-1; i >= 0; i--){
        int k = n >> i;
        s += (k & 1) ? "1" : "0"; //prob unnecessary to use a ternery operator here but meh I like my code compact
    }
    return s;
}

int main(int argc, char *argv[])
{
    std::string line, x;
    getline(std::cin, x);
    const int numOfSymbols = std::stoi(x); //check if this line works!!
    std::vector<std::pair<char, int>> m; //vector of pairs will act as an unordered_map in order to retain the input order of key-value pairs

    while(getline(std::cin, line)){     
        std::string s = line.substr(2);
        int val = std::stoi(s);
        m.push_back(std::make_pair(line[0], val));
    }

    //calculating the number of bits from the fixed-length code
    const auto maxp = std::max_element(m.begin(), m.end(), [](const auto& a, const auto& b) { return a.second < b.second; });
    static int numOfBits = std::ceil(std::log2(maxp->second + 1));
    
    //filling a dynamic array of binvals, each index corresponds to an index of our unordered map
    std::string* binvals = new std::string[numOfSymbols];
    for(int i = 0; i < numOfSymbols; i++)
        binvals[i] = bin(m.at(i).second, numOfBits); 

    int sockfd, newsockfd, portno, clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cout << "ERROR opening socket" << std::endl;
        exit(1);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) { //binds a socket to an address (the server address and port)
        std::cout << "ERROR on binding" << std::endl;
        exit(1);
    } 

    listen(sockfd,10); //allows the process to listen on the socket for connections, NOTE: I changed this from 5 to 10

    clilen = sizeof(cli_addr);
    signal(SIGCHLD, fireman);           
    //accept causes the process to block until a client connects to the server. It's waiting for a connection request from the client
    while((newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t *)&clilen)) >= 0){
        if(fork() == 0) {
            if (newsockfd < 0) {
                std::cout << "ERROR on accept" << std::endl;
                exit(1);
            }

            if (write(newsockfd, &numOfBits, sizeof(int)) < 0){
                std::cout << "ERROR writing to socket (number of bits)" << std::endl;
                exit(1);
            }

            if (read(newsockfd,buffer, 256) < 0) {
                std::cout << "ERROR reading from socket" << std::endl;
                exit(1);
            } 

            char c = 'z';
            for(int i = 0; i < numOfSymbols; i++)
                if(binvals[i] == buffer)
                    c = m.at(i).first;

            if(write(newsockfd,&c,sizeof(char)) < 0){
                std::cerr << "ERROR writing from socket" << std::endl;
                exit(1);
            }
            close(newsockfd);
            _exit(0);
        }
        std::cin.get();
    }
    close(sockfd); 
    return 0;
}
