#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>

long mstime(){
    struct timeval time;
    gettimeofday(&time, NULL);
    return time.tv_sec * 1000000 + time.tv_usec;
}

int buffersize(char * word){
    int k = 0;
    for(int lit = 0; lit < strlen(word);lit++){
        if(word[lit] == 'a') k++;
    }
    return k;
}

int main(int argc, char **argv) {
        short port = 5850;
        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_port = htons(port);
        int sockt = socket(AF_INET, SOCK_DGRAM, 0);
        if(sockt < 0){
            perror("Socket creation failed.");
            exit(1);
        }
        if(argc == 1){
            printf("Server is running!\n");
            address.sin_addr.s_addr = htonl(INADDR_ANY);
            if(bind(sockt, (struct sockaddr *)&address, sizeof(address)) < 0){
                perror("Failed to bind socket");
                exit(1);
            }
            printf("Listening\n");;
            char buffer[1024];
            struct sockaddr_in clientaddress;
            int status;
            int length = sizeof(clientaddress);
            while (1){
                status = recvfrom(sockt, buffer, 1024, 0, (struct sockaddr*)&clientaddress, &length);
                if(status < 0){
                    perror("Failed to get message.");
                    exit(1);
                }
                else {
                    status = sendto(sockt, buffer, status, 0, (struct sockaddr *) &clientaddress, sizeof(clientaddress));
                    if(status < 0){
                        printf("Failed to send message.");
                        exit(1);
                    }
                }
            }
        }
        else if (argc == 5) {
            short size = (short)atoi(argv[2]);
            short package_per_second = atoi(argv[3]);
            short duration = atoi(argv[4]);
            if (address.sin_addr.s_addr == INADDR_NONE) {
                perror("Incorrect ip\n");
                exit(1);
            } else if (size < 100 || size > 1000) {
                perror("Packet size incorrect, it varies in [100 Bytes : 1000 Bytes]\n");
                exit(1);
            } else if (package_per_second > 1000) {
                perror("Incorrect amount packages per second, it cant exceed 1000 packets\n");
                exit(1);
            } else if (duration < 5 || duration > 60) {
                perror("Duration of test varies in [5s : 60s]\n");
                exit(1);
            } else {
                printf("Client running \nIP:%s\nPacket size:%d\nPacket per second:%d\nTest duration:%d\n", argv[1], size, package_per_second, duration);
                address.sin_addr.s_addr = inet_addr(argv[1]);
                long temp = 0;
                long maxtemp = 0;
                long clocksumm = 0;
                int status = 0;
                char buffer[size];
                int len = sizeof(address);
                int second_delay = 1000000 / package_per_second;
                int seconds_delay = 1000000 - second_delay;
                for (int seconds = duration; seconds > 0; seconds--) {
                    printf("%i Second(s) remain\n ", seconds);
                    for (int second = 0; second < package_per_second; second++) {
                        temp = mstime();
                        status = sendto(sockt, buffer, strlen(buffer), 0, (struct sockaddr *) &address,sizeof(address));
                        status = recvfrom(sockt, buffer, size, 0, (struct sockaddr *) &address, &len);
                        temp = mstime() - temp;
                        if (status == 0) {
                            perror("Server is unreachable");
                            exit(1);
                            }
                        if (temp > maxtemp){
                            maxtemp = temp;
                        }
                        clocksumm += temp;
                    }
                    usleep(seconds_delay);
                }
                printf("Maximum delay is:%fms, Average delay is:%fms\n", (float)maxtemp / 2000000, (float)(clocksumm / (package_per_second * duration)) / 2000000);
            }
        }
        else {
            printf("Incorrect input. To run client you need enter:\n<IP> <Packet size> <Packet per second> <Test duracity>\n");
        }
    return 0;
}
