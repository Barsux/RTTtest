#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <fcntl.h>

struct packet{
    long upcoming;
    long incoming;
};

long mstime(){
    struct timeval time;
    gettimeofday(&time, NULL);
    return time.tv_sec * 1000000 + time.tv_usec;
}


int main(int argc, char **argv) {
    short port = 5850;
    struct timeval timeout;
    fd_set sockets;
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
        int status, length = sizeof(clientaddress), allisbroken;
        while (1){
            fd_set sockets;
            timeout.tv_sec = 0;
            timeout.tv_usec = 1000;
            FD_SET(sockt, &sockets);
            status = select(sockt + 1, &sockets, NULL, NULL, &timeout);
            if(status > 0 && FD_ISSET(sockt, &sockets)) {
                status = recvfrom(sockt, buffer, 1024, 0, (struct sockaddr*)&clientaddress, &length);
            }
            else{
                allisbroken++;
            }
            FD_SET(sockt, &sockets);
            status = select(sockt + 1, NULL, &sockets, NULL, &timeout);
            if(status > 0 && FD_ISSET(sockt, &sockets)) {
                status = sendto(sockt, buffer, status, 0, (struct sockaddr *) &clientaddress, sizeof(clientaddress));
            }
            else{
                if(allisbroken == 100){
                    printf("Something goes wrong with connection or client side");
                    exit(EXIT_FAILURE);
                }
                allisbroken++;
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
            short server_stuck = 0, status = 0, index = 0, packet_loss = 0;
            const int second_delay = 1000000 / package_per_second, seconds_delay = 1000000 - second_delay, packet_amount = package_per_second * duration;
            int delay = 0, len = sizeof(address), accepted_packets;
            long temp = 0, maxdelay = 0, clocksumm = 0;
            char buffer[size];
            for(int lit = 0; lit < size; lit++){
                buffer[lit] = 'a';
            }
            long packets[packet_amount];
            bzero(packets, packet_amount);
            timeout.tv_sec = 0;
            timeout.tv_usec = second_delay;
            for (int seconds = duration; seconds > 0; seconds--) {
                printf("%i Second(s) remain\n", seconds);\
                accepted_packets = 0;
                for (int second = 0; second < package_per_second; second++) {
                    temp = 0;
                    index = (duration - seconds) * package_per_second + second;
                    packets[index] = mstime();
                    for(int i = 3; i >= 0; i--){
                        buffer[i] = index % 10 + 48;
                        index /= 10;
                    }
                    FD_SET(sockt, &sockets);
                    status = select(sockt + 1, NULL, &sockets, NULL, &timeout);
                    if(status > 0 && FD_ISSET(sockt, &sockets)) {
                        status = sendto(sockt, buffer, strlen(buffer), 0, (struct sockaddr *) &address,
                                        sizeof(address));
                    }
                    FD_SET(sockt, &sockets);
                    status = select(sockt + 1, &sockets, NULL, NULL, &timeout);
                    if(status > 0 && FD_ISSET(sockt, &sockets)) {
                        accepted_packets++;
                        server_stuck = 0;
                        status = recvfrom(sockt, buffer, size + 1, 0, (struct sockaddr *) &address, &len);
                        temp = mstime();
                        index = (buffer[0] - 48) * 1000 + (buffer[1] - 48) * 100 + (buffer[2] - 48) * 10 + buffer[3] - 48;
                        if(packets[index] > 1000000){
                            packets[index] = temp - packets[index];
                        }
                    }
                    else{
                        if(server_stuck == package_per_second){
                            perror("Server isn't responding.\n");
                            exit(EXIT_FAILURE);
                        }
                        server_stuck++;
                    }
                }
                for(int i = 0; i < package_per_second - accepted_packets; i++){
                    FD_SET(sockt, &sockets);
                    status = select(sockt + 1, &sockets, NULL, NULL, &timeout);
                    if(status > 0 && FD_ISSET(sockt, &sockets)) {
                        accepted_packets++;
                        server_stuck = 0;
                        status = recvfrom(sockt, buffer, size + 1, 0, (struct sockaddr *) &address, &len);
                        temp = mstime();
                        index = (buffer[0] - 48) * 1000 + (buffer[1] - 48) * 100 + (buffer[2] - 48) * 10 + buffer[3] - 48;
                        if(packets[index] > 1000000){
                            packets[index] = temp - packets[index];
                        }
                    }
                }
                usleep(accepted_packets * second_delay);
            }
            for(int i = 0; i < package_per_second * duration; i++){
                if(packets[i] < 1000000){
                    if(packets[i] > maxdelay) {
                        maxdelay = packets[i];
                    }
                    clocksumm += packets[i];
                }
                else {
                    packet_loss++;
                }
            }
            printf("Maximum delay is:%fms, Average delay is:%fms, Packet loss is:%i%%\n", (float)maxdelay / 2000, (float)(clocksumm / (package_per_second * duration - packet_loss)) / 2000, (int)((float)packet_loss / packet_amount * 100));
        }}
    else {
        printf("Incorrect input. To run client you need enter:\n<IP> <Packet size> <Packet per second> <Test duracity>\n");
    }
    return 0;
}