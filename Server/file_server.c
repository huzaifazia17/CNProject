#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define SERVER_TCP_PORT 3000
#define BUFLEN          101

// Function prototype for sending a file
void send_file(int sd, char *filename);

int main(int argc, char **argv) {
    int sd, new_sd, client_len, port;
    struct sockaddr_in server, client;
    char filename[BUFLEN - 1];

    // Set the port based on the command line argument, if provided
    port = (argc == 2) ? atoi(argv[1]) : SERVER_TCP_PORT;

    // Create a TCP socket
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Can't create a socket");
        exit(1);
    }

    // Bind the socket to an address and port
    bzero((char *) &server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sd, (struct sockaddr *) &server, sizeof(server)) == -1) {
        perror("Can't bind name to socket");
        exit(1);
    }

    // Listen for incoming connections
    listen(sd, 5);

    while (1) {
        client_len = sizeof(client);
        new_sd = accept(sd, (struct sockaddr *) &client, &client_len);
        if (new_sd < 0) {
            perror("Can't accept client");
            exit(1);
        }

        // Receive the filename from the client
        int recv_len = recv(new_sd, filename, BUFLEN - 1, 0);
        if (recv_len > 0) {
            filename[recv_len] = '\0';
            send_file(new_sd, filename);
        }

        close(new_sd);
        break;
    }

    close(sd);
    return 0;
}

// Function to send the file or an error message if the file can't be opened
void send_file(int sd, char *filename) {
    FILE *fp = fopen(filename, "rb");
    char buffer[BUFLEN];
    int bytes_read;

    if (fp != NULL) {
        buffer[0] = 'F'; // File data indicator
        while ((bytes_read = fread(buffer + 1, 1, BUFLEN - 1, fp)) > 0) {
            send(sd, buffer, bytes_read + 1, 0);
        }
        fclose(fp);
    } else {
        buffer[0] = 'E'; // Error message indicator
        strcpy(buffer + 1, "Error: Could not open the file.");
        send(sd, buffer, strlen(buffer + 1) + 1, 0);
    }
}
