#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define SERVER_PORT 9000

#define MAX_BUFFER 100

struct pdu {
    char type;
    char data[MAX_BUFFER];
};

void send_file(int sockfd, struct sockaddr_in *client_addr, socklen_t client_len, char *filename);

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    struct pdu rcv_pdu;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(1);
    }

    printf("Server is running...\n");

    while (1) {
        client_len = sizeof(client_addr);
        recvfrom(sockfd, &rcv_pdu, sizeof(rcv_pdu), 0, (struct sockaddr *)&client_addr, &client_len);

        if (rcv_pdu.type == 'C') {
            send_file(sockfd, &client_addr, client_len, rcv_pdu.data);
        }
    }

    close(sockfd);

    return 0;
}

void send_file(int sockfd, struct sockaddr_in *client_addr, socklen_t client_len, char *filename) {
    struct pdu snd_pdu;
    int file_fd;
    ssize_t bytes_read;
    struct stat file_stat;

    file_fd = open(filename, O_RDONLY);
    if (file_fd < 0) {
        perror("open");
        snd_pdu.type = 'E';
        snprintf(snd_pdu.data, MAX_BUFFER, "Error opening file: %s", strerror(errno));
        sendto(sockfd, &snd_pdu, sizeof(snd_pdu), 0, (struct sockaddr *)client_addr, client_len);
        return;
    }

    fstat(file_fd, &file_stat);
    off_t remaining_size = file_stat.st_size;

    while ((bytes_read = read(file_fd, snd_pdu.data, MAX_BUFFER)) > 0) {
        snd_pdu.type = (remaining_size <= MAX_BUFFER) ? 'F' : 'D';
        sendto(sockfd, &snd_pdu, bytes_read + 1, 0, (struct sockaddr *)client_addr, client_len);
        remaining_size -= bytes_read;
    }

    close(file_fd);
}
