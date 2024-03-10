#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <errno.h>

#define BUFFER_SIZE 1024 * 1024
#define FILE_PATH "random_file.txt"

char *IP;   // IP address of the server
int PORT;   // Port number of the server
char *ALGO; // Congestion control algorithm to be used

// Message types
enum MessageType {
    FILE_DATA,       // Represents file data
    CONTROL_MESSAGE  // Represents control messages
};

// Struct to represent messages
typedef struct {
    enum MessageType type;  // Type of message
    char data[BUFFER_SIZE]; // Data of the message
    size_t length;          // Length of the message data
} Message;

/**
 * Extracts IP address, port number, and congestion control algorithm from command line arguments.
 *
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments
 * @return 0 if extraction is successful, 1 otherwise
 */
int extract_Variables(int argc, char *argv[]) {
    if (argc != 7 || strcmp(argv[1], "-ip") != 0 || strcmp(argv[3], "-p") != 0 || strcmp(argv[5], "-algo") != 0) {
        printf("Usage: %s -ip IP -p Port -algo Algo\n", argv[0]);
        return 1; // Exit with error
    }

    IP = argv[2];
    PORT = atoi(argv[4]);
    ALGO = argv[6];

    return 0;
}

/**
 * Sets the congestion control algorithm for the socket.
 *
 * @param server_fd File descriptor of the socket
 */
void set_congestion_control(int server_fd) {
    int result;
    if (strcmp(ALGO, "reno") == 0) {
        const char *congestion_algo = "reno";
        result = setsockopt(server_fd, IPPROTO_TCP, TCP_CONGESTION, congestion_algo, strlen(congestion_algo));
    } else if (strcmp(ALGO, "cubic") == 0) {
        const char *congestion_algo = "cubic";
        result = setsockopt(server_fd, IPPROTO_TCP, TCP_CONGESTION, congestion_algo, strlen(congestion_algo));
    } else {
        printf("Invalid congestion control algorithm.\n");
        exit(EXIT_FAILURE);
    }

    if (result < 0) {
        perror("setsockopt TCP_CONGESTION failed");
        exit(EXIT_FAILURE);
    }
}

/**
 * Sends a message through the socket.
 *
 * @param socket File descriptor of the socket
 * @param msg Pointer to the Message structure to be sent
 */
void send_message(int socket, const Message *msg) {
    ssize_t bytes_sent = send(socket, msg, sizeof(Message), 0);
    if (bytes_sent != sizeof(Message)) {
        perror("Error sending message");
        exit(EXIT_FAILURE);
    }
}

/**
 * Sends a control message through the socket.
 *
 * @param socket File descriptor of the socket
 * @param msg The control message to be sent
 */
void send_control_message(int socket, const char *msg) {
    Message control_msg;
    control_msg.type = CONTROL_MESSAGE;
    strncpy(control_msg.data, msg, BUFFER_SIZE);
    control_msg.length = strlen(msg) + 1; // +1 to include the null terminator

    send_message(socket, &control_msg);
}

/**
 * Sends the content of a file through the socket.
 *
 * @param socket File descriptor of the socket
 */
void send_file(int socket) {
    FILE *file = fopen(FILE_PATH, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    // Send "START" message before sending the file
    Message start_msg;
    start_msg.type = CONTROL_MESSAGE;
    strcpy(start_msg.data, "START");
    start_msg.length = strlen(start_msg.data);
    send_message(socket, &start_msg);

    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        Message file_msg;
        file_msg.type = FILE_DATA;
        memcpy(file_msg.data, buffer, bytes_read);
        file_msg.length = bytes_read;
        send_message(socket, &file_msg);
    }

    // Send "END" message after finishing sending the file
    Message end_msg;
    end_msg.type = CONTROL_MESSAGE;
    strcpy(end_msg.data, "END");
    end_msg.length = strlen(end_msg.data);
    send_message(socket, &end_msg);

    fclose(file);
}

/**
 * Creates a socket.
 *
 * @return File descriptor of the created socket
 */
int create_socket() {
    int sock;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        exit(EXIT_FAILURE);
    }
    return sock;
}

/**
 * Connects to the server.
 *
 * @param sock File descriptor of the socket
 */
void connect_to_server(int sock) {
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET; // set the address family to AF_INET (IPv4)
    serv_addr.sin_port = htons(PORT); // set the port number
    if (inet_pton(AF_INET, IP, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for TCP connection...\n");

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        exit(EXIT_FAILURE);
    }
}

/**
 * Closes the socket.
 *
 * @param sock File descriptor of the socket
 */
void close_socket(int sock) {
    close(sock);
}

/**
 * Main function.
 *
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments
 * @return 0 indicating successful execution of the program
 */
int main(int argc, char *argv[]) {
    if (extract_Variables(argc, argv) == 1) {
        return 1;
    }
    printf("Starting Sender...\n");

    int sock = create_socket();
    set_congestion_control(sock);

    system("./File_Generator"); // Assuming File_Generator is a separate program to generate random_file.txt
    connect_to_server(sock);

    printf("Connection established. Sending file...\n");

    while (1) {
        send_file(sock);
        printf("File sent successfully.\n");
        // Prompt user to send the file again
        char response[10];
        printf("Do you want to send the file again? (yes/no): ");
        scanf("%s", response);

        if (strcmp(response, "no") == 0 || strcmp(response, "n") == 0) {
            send_control_message(sock, "EXIT");
            break;
        }

        // Send "SEND_AGAIN" message
        send_control_message(sock, "SEND_AGAIN");
    }
    close_socket(sock);
    return 0;
}
