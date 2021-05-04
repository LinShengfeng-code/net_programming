#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<sys/time.h>
#include<sys/select.h>

#include "calcLib.h"
#include"protocol.h"

int main(int argc, char *argv[])
{
    int retransmit = 0;
    int state = 0;// 0 start; 1 return calc result
    //socket
    int clitfd = socket(AF_INET, SOCK_DGRAM, 0);
    char serverAdd[20] = "13.53.76.30";
    char *serverAddPtr = &serverAdd[0];
    char serverPort[5] = "5000";
    char *serverPortPtr = &serverPort[0];
    struct sockaddr_in servadd;
    memset(&servadd,0,sizeof(servadd));
    servadd.sin_family = AF_INET;
    servadd.sin_port = htons(atoi(serverPortPtr));
    servadd.sin_addr.s_addr = inet_addr(serverAddPtr);

    struct calcMessage sendMessage,returnMessage;
    struct calcProtocol respondeMessage;
    char rvbuf[1024];
    //first calcMessage
    sendMessage.type = htons(22);
    sendMessage.message = htonl(0);
    sendMessage.protocol = htons(17);
    sendMessage.major_version = htons(1);
    sendMessage.minor_version = htons(0);

    sendto(clitfd,&sendMessage,sizeof(sendMessage),0,(struct sock_addr*)&servadd, sizeof(servadd));
    printf("First calcMessage has been sent.\n  type: %hu\n", ntohs(sendMessage.type));
    // prepare for select
    int maxfdp;
    fd_set fds;
    struct timeval timeout = {2, 0};

    while(1)
    {
        FD_ZERO(&fds);//clean up.
        FD_SET(clitfd, &fds);
        maxfdp = clitfd + 1;
        int num = select(maxfdp, &fds, NULL, NULL, &timeout);
        if(num == -1){
            printf("Select Error\n");
            break;
        }
        else if(num == 0){
            if(retransmit == 2){
                printf("transmit times = 2\n");
                break;
            }
            timeout.tv_sec = 2;
            timeout.tv_usec = 0;
            printf("Datagram hasn't been sent.\nResend right now.\n");
            if(state == 0){
                sendto(clitfd,&sendMessage,sizeof(sendMessage),0,(struct sock_addr*)&servadd, sizeof(servadd));
                printf("First calcMessage has been sent.\n  type: %hu\n", ntohs(sendMessage.type));
            }
            else{//state == 1
                sendto(clitfd, &respondeMessage, sizeof(respondeMessage), 0, (struct sock_addr*)&servadd, sizeof(servadd));
            }
            retransmit++;
            continue;
        }
        else{
            retransmit = 0;
            int ret = recvfrom(clitfd, rvbuf, sizeof(rvbuf), 0, NULL, NULL);
                if(ret == -1)
                {
                    printf("Receive an error.\n");
                    break;
                }
                else if(ret == 50)
                {
                    memcpy(&respondeMessage, rvbuf, ret);
                    printf("From Server:\n  type: %hu\n arith: %u\n inValue1: %u\n   inValue2: %u\n flValue1: %f\n   flValue2: %f\n", 
                    ntohs(respondeMessage.type),ntohl(respondeMessage.arith),ntohl(respondeMessage.inValue1),ntohl(respondeMessage.inValue2),respondeMessage.flValue1,respondeMessage.flValue2);
                    switch(ntohl(respondeMessage.arith)){
                        case 1:
                            respondeMessage.inResult = htonl(ntohl(respondeMessage.inValue1)+ntohl(respondeMessage.inValue2));
                            break;
                        case 2:
                            respondeMessage.inResult = htonl(ntohl(respondeMessage.inValue1)-ntohl(respondeMessage.inValue2));
                            break;
                        case 3:
                            respondeMessage.inResult = htonl(ntohl(respondeMessage.inValue1)*ntohl(respondeMessage.inValue2));
                            break;
                        case 4:
                            respondeMessage.inResult = htonl(ntohl(respondeMessage.inValue1)/ntohl(respondeMessage.inValue2));
                            break;
                        case 5:
                            respondeMessage.flResult = respondeMessage.flValue1+respondeMessage.flValue2;
                            break;
                        case 6:
                            respondeMessage.flResult = respondeMessage.flValue1-respondeMessage.flValue2;
                            break;
                        case 7:
                            respondeMessage.flResult = respondeMessage.flValue1*respondeMessage.flValue2;
                            break;
                        case 8:
                            respondeMessage.flResult = respondeMessage.flValue1/respondeMessage.flValue2;
                            break;
                    }
                    sendto(clitfd, &respondeMessage, sizeof(respondeMessage), 0, (struct sock_addr*)&servadd, sizeof(servadd));
                    state = 1;
                    printf("New calcProtocol has been sent.\n   inResult: %u\n  flResult: %lf\n", ntohl(respondeMessage.inResult), respondeMessage.flResult);
                   continue;
                }
                else if(ret == 12)
                {
                    memcpy(&returnMessage,rvbuf, ret);
                    if(state == 0){
                        printf("CONNECTION REJECTED.\n");
                        break;
                    }
                    else{
                         char OKState[3] = "OK";
                        char notOKState[7] = "NOT OK";
                        char naState[4] = "N/A";
                        printf("return state is %u\n", ntohl(returnMessage.message));
                        if(ntohl(returnMessage.message)==1)printf("Server returns %s\n", OKState);
                        else if(ntohl(returnMessage.message)==2)printf("Server returns %s\n", notOKState);
                        else printf("Server returns %s\n", naState);
                        break;
                    }
                }
        }        
    }
    close(clitfd);
    return 0;
}