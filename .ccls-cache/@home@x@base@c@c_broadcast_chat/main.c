#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_MSG_LEN 1000

struct thread_args {
    int sockfd;
    struct sockaddr_in broadcast_addr;
    char nickname[20];
};

void *recv_thread(void *arg) {
    struct thread_args *args = (struct thread_args *)arg;
    char msg[MAX_MSG_LEN];
    struct sockaddr_in sender_addr;
    socklen_t addrlen = sizeof(sender_addr);
    while (1) {
        int len = recvfrom(args->sockfd, msg, MAX_MSG_LEN, 0,
                           (struct sockaddr *)&sender_addr, &addrlen);
        if (len > 0) {
            msg[len] = '\0';
            printf("%s (%s): %s\n", inet_ntoa(sender_addr.sin_addr),
                   msg, args->nickname);
        }
    }
}

void *send_thread(void *arg) {
    struct thread_args *args = (struct thread_args *)arg;
    char msg[MAX_MSG_LEN];
    while (1) {
        fgets(msg, MAX_MSG_LEN, stdin);
        msg[strlen(msg)-1] = '\0'; // remove newline
        char buf[MAX_MSG_LEN+30];
        sprintf(buf, "%s: %s", args->nickname, msg);
        sendto(args->sockfd, buf, strlen(buf), 0,
               (struct sockaddr *)&(args->broadcast_addr),
               sizeof(args->broadcast_addr));
    }
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <ip_address> <port>\n", argv[0]);
        exit(1);
    }

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    int broadcast_enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST,
                   &broadcast_enable, sizeof(broadcast_enable)) < 0) {
        perror("setsockopt");
        exit(1);
    }

    struct sockaddr_in broadcast_addr;
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(atoi(argv[2]));
    broadcast_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    if (bind(sockfd, (struct sockaddr *)&broadcast_addr,
             sizeof(broadcast_addr)) < 0) {
        perror("bind");
        exit(1);
    }

    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(atoi(argv[2]));
    if (inet_aton(argv[1], &(local_addr.sin_addr)) == 0) {
        fprintf(stderr, "Invalid IP address\n");
        exit(1);
    }

    if (connect(sockfd, (struct sockaddr *)&local_addr,
                sizeof(local_addr)) < 0) {
        perror("connect");
        exit(1);
    }

    printf("Enter your nickname: ");
    char nickname[20];
    fgets(nickname, 20, stdin);
    nickname[strlen(nickname)-1] = '\0'; // remove newline

    pthread_t tid_recv, tid_send;
    struct thread_args args = {sockfd, broadcast_addr};
    strcpy(args.nickname, nickname);
    pthread_create(&tid_recv, NULL, recv_thread, &args);
    pthread_create(&tid_send, NULL, send_thread, &args);

    pthread_join(tid_recv, NULL);
    pthread_join(tid_send, NULL);

    close(sockfd);
    return 0;
}
