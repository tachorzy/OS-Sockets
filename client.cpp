#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <pthread.h>
#include <cstring>

struct args { //args for the frequency function
    std::string key, decomp_msg, msg, binval;
    char buffer[256];
    char c;
    int size, bits, portno;
    struct hostent* server;
};

void* decompress(void* void_ptr){ 
    struct args *ptr = (struct args*)void_ptr;
    struct sockaddr_in serv_addr;
    struct hostent* server = ptr->server;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        std::cerr << "ERROR: opening socket" << std::endl;
        exit(1);
    }
    if (server == NULL) {
        std::cerr << "ERROR: no such host" << std::endl;
        exit(1);
    }

    serv_addr.sin_family = AF_INET;

    bcopy((char *)server->h_addr,(char *)&serv_addr.sin_addr.s_addr,server->h_length);
    serv_addr.sin_port = htons(ptr->portno);
    
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0){
        std::cerr << "ERROR connecting" << std::endl;
        exit(1);
    } 
    
    const char* temp = ptr->key.c_str();

    bzero(ptr->buffer, 256);
    for(int i = 0; i < ptr->bits; i++){
        ptr->buffer[i] = temp[i];
    }

    int b;
    if(read(sockfd,&b,sizeof(int)) < 0){
        std::cerr << "ERROR: connecting socket" << std::endl;
        exit(1);
    }

    if(write(sockfd,ptr->buffer,256) < 0) { 
        std::cerr << "ERROR connecting socket (decompressed message)" << std::endl;
        exit(1);
    }

    if(read(sockfd,&ptr->c,sizeof(char)) < 0){
        std::cerr << "ERROR: connecting socket" << std::endl;
        exit(1);
    }

    ptr->decomp_msg += ptr->c;
    close(sockfd);
    return NULL;
}

//this code was heavily informed/inspired by the sockets tutorial by Robert Ingalls and his client.c file: https://www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/socket.html
int main(int argc, char* argv[]){ //or alternatively char** argv
    //getting the compressed message.
    std::string comp_msg;
    std::getline(std::cin, comp_msg);
    int sockfd, portno, n;
    
    struct sockaddr_in serv_addr;
    struct hostent* server; //points to a hostent struct which will have all of the host info (name, aliases, address list, etc)

    char buffer[256]; //server reads characters from the socket connection into this buffer
    //checks the arguments passed: 
    if (argc < 3) {  //if number of args is less than 3 there's a problem here
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(1);
    }

    server = gethostbyname(argv[1]); //argv[1] contains the name of the host. We pass this into gethostbyname which returns a *hostent containing the host's info
    if(server == NULL) { //checking if we actually received the host's info
        std::cerr << "ERROR: system did not locate a host with that name." << std::endl;
        exit(1);
    }

    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0); //creating the file descriptor, attached later to the IP address of the server
    if(sockfd < 0){
        std::cerr << "ERROR: opening socket" << std::endl;
        exit(1);
    }
    //source: Robert Ingalls [lines 33-39]
    bzero((char *) &serv_addr, sizeof(serv_addr)); //bzero will initialize all values in the buffer to 0, &serv_addr is pointing to the buffer, and second arg is the size of the buffer 
    serv_addr.sin_family = AF_INET; //set the family as AF_INET (same family we are using for our socket)
    bcopy((char *)server->h_addr, &serv_addr.sin_addr.s_addr, server->h_length);  //copies information from param1 to param2 by the given length of bytes (param3)(char *)\p
    serv_addr.sin_port = htons(portno); //assign the port number using the hton family func

    if(connect(sockfd,(struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
        std::cerr << "ERROR: connection could not be made" << std::endl;
        exit(1);
    }

    //lines 102 to 115: sending a request to the server to know the number of bits
    //receive the number of bits through the socket from the server, remember that n is an int 
    bzero(buffer,256);
    if(write(sockfd,buffer,strlen(buffer)) < 0){
        std::cerr << "ERROR: error writing to socket" << std::endl;
        exit(1);
    }

    int numOfBits = 0;
    if(read(sockfd, &numOfBits, sizeof(int)) < 0){
        std::cerr << "ERROR: error reading from socket" << std::endl;
        exit(1);
    }

    const int MTHREADS = comp_msg.length() / numOfBits;
    pthread_t* tid = new pthread_t[MTHREADS];
    static struct args* x = new args[MTHREADS];
    //fill from the the unordered map to x
    x->key = new char[MTHREADS];
    int i = 0;
    for(int j = 0; j < comp_msg.length(); j += numOfBits){
        x[i].key = comp_msg.substr(j, numOfBits);
        i++;
    }

    for(int i = 0; i < MTHREADS; i++){
        x[i].bits = numOfBits;
        x[i].size = MTHREADS;
        x[i].msg = comp_msg;
        x[i].portno = atoi(argv[2]);
        x[i].server = server;
    }
            
    for(int i = 0; i < MTHREADS; i++){
        if(pthread_create(&tid[i], NULL, decompress, &x[i])){
            std::cout << "error creating thread:" << stderr << std::endl;
            return 1;
        }
    }

    for(int i = 0; i < MTHREADS; i++) {
        pthread_join(tid[i], NULL);
    }
    close(sockfd);

    //printing down here:
    std::cout << "Decompressed message: ";
    for(int i = 0; i < MTHREADS; i++)
        std:: cout << x[i].c;
    std::cout << std::endl;

    delete[] tid;
    delete[]x;
    return 0;
}