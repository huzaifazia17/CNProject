#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_PORT 9000
#define BUFLEN 100

struct pdu {
    char type;
    char data[BUFLEN];
};

void download_file(int sockfd, struct sockaddr_in *server_addr, char *filename);

int main(int argc, char **argv) {
    int sockfd;
    struct sockaddr_in server_addr;
    char filename[BUFLEN];

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <server_ip>\n", argv[0]);
        exit(1);
    }

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        exit(1);
    }

    while (1) {
        printf("Enter filename to download or 'q' to quit: ");
        fgets(filename, sizeof(filename), stdin);
        size_t len = strlen(filename);
        if (len > 0 && filename[len - 1] == '\n') {
            filename[len - 1] = '\0';
        }

        if (strcmp(filename, "q") == 0) {
            break;
        }

        download_file(sockfd, &server_addr, filename);
    }

    close(sockfd);
    return 0;
}

void download_file(int sockfd, struct sockaddr_in *server_addr, char *filename) {
    struct pdu snd_pdu, rcv_pdu;
    FILE *fp;
    int server_addr_len = sizeof(*server_addr);

    snd_pdu.type = 'C';
    strncpy(snd_pdu.data, filename, sizeof(snd_pdu.data));
    sendto(sockfd, &snd_pdu, strlen(filename) + 1, 0, (struct sockaddr *) server_addr, server_addr_len);

    fp = fopen(filename, "wb");
    if (!fp) {
        perror("fopen");
        return;
    }

    while (1) {
        int recv_len = recvfrom(sockfd, &rcv_pdu, sizeof(rcv_pdu), 0, (struct sockaddr *) server_addr, &server_addr_len);
        if (recv_len < 0) {
            perror("recvfrom");
            break;
        }

        if (rcv_pdu.type == 'D' || rcv_pdu.type == 'F') {
            fwrite(rcv_pdu.data, 1, recv_len - 1, fp);
        } else if (rcv_pdu.type == 'E') {
            fprintf(stderr, "Error: %s\n", rcv_pdu.data);
            break;
        }

        if (rcv_pdu.type == 'F') {
            printf("File download complete.\n");
            break;
        }
    }

    fclose(fp);
}
