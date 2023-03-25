#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>

#define SERVER_TCP_PORT 3000
#define BUFLEN          101

int main(int argc, char **argv) {
    int sd, port, bytes_received;
    struct hostent *hp;
    struct sockaddr_in server;
    char *host, filename[BUFLEN - 1], buffer[BUFLEN];
    FILE *output_file;

    // Set the host and port based on command line arguments, if provided
    host = (argc >= 2) ? argv[1] : "localhost";
    port = (argc == 3) ? atoi(argv[2]) : SERVER_TCP_PORT;

    // Create a TCP socket
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Can't create a socket");
        exit(1);
    }

    // Set up the server address structure
    bzero((char *)&server, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    if (hp = gethostbyname(host)) {
        bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);
    } else if (inet_aton(host, (struct in_addr *)&server.sin_addr)) {
        perror("Can't get server's address");
        exit(1);
    }

    // Connect to the server
    if (connect(sd, (struct sockaddr *)&server, sizeof(server)) == -1) {
        perror("Can't connect");
        exit(1);
    }

    // Get the filename from the user
    printf("Enter the filename to download: ");
    fgets(filename, BUFLEN - 1, stdin);
    filename[strcspn(filename, "\n")] = '\0'; // Remove the newline character

    // Send the filename to the server
    send(sd, filename, strlen(filename), 0);

    // Receive the file data or error message from the server
    bytes_received = recv(sd, buffer, BUFLEN, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';

        // Check if the received data is a file or an error message
        if (buffer[0] == 'F') {
            // Save the file data into a local file named "downloaded_file"
            output_file = fopen("downloaded_file", "wb");
            if (output_file == NULL) {
                perror("Error creating output file");
                exit(1);
            }

            fwrite(buffer + 1, 1, bytes_received - 1, output_file);
            while ((bytes_received = recv(sd, buffer, BUFLEN, 0)) > 0) {
                fwrite(buffer, 1, bytes_received, output_file);
            }
            fclose(output_file);
            printf("File downloaded successfully.\n");
        } else if (buffer[0] == 'E') {
            // Display the error message in the terminal window
            printf("%s\n", buffer + 1);
        } else {
            printf("Unexpected response from the server.\n");
        }
    } else {
        printf("No response from the server.\n");
    }

    // Close the socket
    close(sd);
    return 0;
}
