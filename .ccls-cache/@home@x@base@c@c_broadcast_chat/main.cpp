#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_MSG_LEN 1000

using namespace std;

struct ThreadData {
    int sockfd;
    sockaddr_in broadcast_addr;
    string nickname;
};

void* receive_thread(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    char buffer[MAX_MSG_LEN];
    sockaddr_in sender_addr;
    socklen_t sender_len = sizeof(sender_addr);
    while (true) {
        int len = recvfrom(data->sockfd, buffer, MAX_MSG_LEN, 0,
                           (sockaddr*)&sender_addr, &sender_len);
        if (len == -1) {
            cerr << "Error receiving message" << endl;
            continue;
        }
        buffer[len] = '\0';
        cout << inet_ntoa(sender_addr.sin_addr) << " (" << buffer << "): " << endl;
    }
}

void* send_thread(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    char buffer[MAX_MSG_LEN];
    while (true) {
        cin.getline(buffer, MAX_MSG_LEN);
        string message = data->nickname + ": " + buffer;
        int len = message.length();
        if (len > MAX_MSG_LEN) {
            cerr << "Message too long" << endl;
            continue;
        }
        if (sendto(data->sockfd, message.c_str(), len, 0,
                   (sockaddr*)&(data->broadcast_addr),
                   sizeof(data->broadcast_addr)) == -1) {
            cerr << "Error sending message" << endl;
            continue;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <ip_address> <port>" << endl;
        exit(1);
    }
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        cerr << "Error creating socket" << endl;
        exit(1);
    }
    int broadcast_enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST,
                   &broadcast_enable, sizeof(broadcast_enable)) == -1) {
        cerr << "Error setting socket option" << endl;
        exit(1);
    }
    sockaddr_in my_addr;
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons(atoi(argv[2]));
    if (bind(sockfd, (sockaddr*)&my_addr, sizeof(my_addr)) == -1) {
        cerr << "Error binding socket" << endl;
        exit(1);
    }
    sockaddr_in broadcast_addr;
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_addr.s_addr = inet_addr("192.168.0.255");
    broadcast_addr.sin_port = htons(atoi(argv[2]));
    cout << "Enter your nickname: ";
    string nickname;
    cin >> nickname;
    ThreadData data = {sockfd, broadcast_addr, nickname};
    pthread_t receive_tid, send_tid;
    pthread_create(&receive_tid, NULL, receive_thread, &data);
    pthread_create(&send_tid, NULL, send_thread, &data);
    pthread_join(receive_tid, NULL);
    pthread_join(send_tid, NULL);
    close(sockfd);
    return 0;
}

