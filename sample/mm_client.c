/**
 *===========================================================================
 *  DarkBlue Engine Source File.
 *  Copyright (C), DarkBlue Studios.
 * -------------------------------------------------------------------------
 *    File name: mm_client.c
 *      Version: v0.0.0
 *   Created on: Apr 2, 2019 by konyka
 *       Editor: Sublime Text3
 *  Description: 
 * -------------------------------------------------------------------------
 *      History:
 *
 *===========================================================================
 */
 

#include "mm_coroutine.h"

#include <arpa/inet.h>



#define mm_SERVER_IPADDR       "127.0.0.1"
#define mm_SERVER_PORT         9096

int init_client(void) {

    int clientfd = mm_socket(AF_INET, SOCK_STREAM, 0);
    if (clientfd <= 0) {
        printf("socket failed\n");
        return -1;
    }

    struct sockaddr_in serveraddr = {0};
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(mm_SERVER_PORT);
    serveraddr.sin_addr.s_addr = inet_addr(mm_SERVER_IPADDR);

    int result = mm_connect(clientfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if (result != 0) {
        printf("connect failed\n");
        return -2;
    }

    return clientfd;
    
}

void client(void *arg) {

    int clientfd = init_client();
    char *buffer = "mmco_client\r\n";

    while (1) {

        int length = mm_send(clientfd, buffer, strlen(buffer), 0);
        printf("echo length : %d\n", length);

        sleep(1);
    }

}



int main(int argc, char *argv[]) {
    mm_coroutine *co = NULL;

    mm_coroutine_create(&co, client, NULL);
    
    mm_schedule_run(); //run

    return 0;
}







