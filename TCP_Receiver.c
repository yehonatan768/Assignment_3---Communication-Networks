#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BUFFER_SIZE 1024 * 1024
#define MKDIR(directory) mkdir(directory, 0700)
#define DIR "assets"

int PORT; // Port number of the server
char *ALGO; // Congestion control algorithm to be used

// Message types
enum MessageType {
    FILE_DATA,
    CONTROL_MESSAGE
};

// Struct to represent messages
typedef struct {
    enum MessageType type;
    char data[BUFFER_SIZE];
    size_t length;
} Message;

// Define the node structure
typedef struct Node {
    double data;
    struct Node *next;
} Node;

// Define the list structure
typedef struct {
    Node *head;
} List;

/**
 * Create a directory if it doesn't exist.
 */
void creating_path() {
    // Use MKDIR macro for directory creation
    if (MKDIR(DIR) != 0) {
        fprintf(stderr, "Error creating directory.\n");
        exit(EXIT_FAILURE);
    }
    printf("Directory created successfully.\n");
}

/**
 * Create an empty list.
 *
 * @return Pointer to the newly created list.
 */
List *createList() {
    List *list = (List *)malloc(sizeof(List));
    if (list == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    list->head = NULL;
    return list;
}

/**
 * Get the size of the list.
 *
 * @param list Pointer to the list.
 * @return Size of the list.
 */
int size(List *list) {
    if (list == NULL) {
        return 0;
    }
    Node *current = list->head;
    int i = 0; // Start count from 0
    while (current != NULL) {
        current = current->next;
        i++;
    }
    return i;
}

/**
 * Insert an element at the end of the list.
 *
 * @param list Pointer to the list.
 * @param value Value to be inserted.
 */
void insert(List *list, double value) {
    Node *newNode = (Node *)malloc(sizeof(Node));
    if (newNode == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    newNode->data = value;
    newNode->next = NULL;

    if (list->head == NULL) {
        list->head = newNode;
    } else {
        Node *current = list->head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newNode;
    }
}

/**
 * Delete an element from the list by value.
 *
 * @param list Pointer to the list.
 * @param value Value to be deleted.
 */
void deleteValue(List *list, double value) {
    Node *current = list->head;
    Node *prev = NULL;

    while (current != NULL) {
        if (current->data == value) {
            if (prev == NULL) {
                list->head = current->next;
            } else {
                prev->next = current->next;
            }
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}

/**
 * Delete the entire list and free memory.
 *
 * @param list Pointer to the list.
 */
void deleteList(List *list) {
    Node *current = list->head;
    while (current != NULL) {
        Node *next = current->next;
        free(current);
        current = next;
    }
    free(list);
}

/**
 * Print the elements of the list.
 *
 * @param list Pointer to the list.
 */
void printList(List *list) {
    Node *current = list->head;
    while (current != NULL) {
        printf("%.2f ", current->data);
        current = current->next;
    }
    printf("\n");
}

/**
 * Print times and average time from a dynamic array.
 *
 * @param iteration Number of iterations.
 * @param times Pointer to the list containing times.
 */
void print_times(int iteration, List *times) {
    printf("____________________________________________________________\n");
    printf("-                     *  statistics  *                     -\n");
    printf("-\n");
    double avg = 0;
    double total_bandwidth = 0;
    Node *current = times->head; // Start from the head of the list
    int i = 1; // Variable to keep track of the run number
    while (current != NULL) {
        printf("- Run   #%d  Data: Time = %.2f ms;    speed = %.2f MB/s\n", i, current->data, 1024.00 / current->data);
        avg += current->data;
        total_bandwidth += 1024.00 / current->data;
        current = current->next; // Move to the next node
        i++; // Increment the run number
    }
    printf("-\n");
    printf("- Average time:   %.2f ms\n", avg / iteration);
    printf("- Average bandwidth:  %.2f MB/s\n", total_bandwidth / iteration);
    printf("____________________________________________________________\n");
}

/**
 * Extract port number and congestion control algorithm from command line arguments.
 *
 * @param argc Number of command line arguments.
 * @param argv Array of command line arguments.
 * @return 0 if extraction is successful, 1 otherwise.
 */
int extract_Variables(int argc, char *argv[]) {
    // Check if the number of arguments is correct
    if (argc != 5 || strcmp(argv[1], "-p") != 0 || strcmp(argv[3], "-algo") != 0) {
        printf("Usage: %s -p PORT -algo ALGO\n", argv[0]);
        return 1; // Exit with error
    }

    // Extract PORT and ALGO from command-line arguments
    PORT = atoi(argv[2]);
    ALGO = argv[4];
    return 0;
}

/**
 * Set the congestion control algorithm for the socket.
 *
 * @param sock File descriptor of the socket.
 */
void set_congestion_control(int sock) {
    const char *congestion_algo = (strcmp(ALGO, "cubic") == 0) ? "cubic" : "reno";
    if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, congestion_algo, strlen(congestion_algo)) < 0) {
        perror("setsockopt TCP_CONGESTION failed");
        exit(EXIT_FAILURE);
    }
}

/**
 * Open a file for writing.
 *
 * @param file_name Name of the file.
 * @return File pointer.
 */
FILE *open_file_to_write(char *file_name) {
    FILE *file = fopen(file_name, "wb");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    return file;
}

/**
 * Create a server socket.
 *
 * @param server_fd Pointer to store the server socket file descriptor.
 */
void create_server_socket(int *server_fd) {
    if ((*server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
}

/**
 * Bind the server socket to an address.
 *
 * @param server_fd File descriptor of the server socket.
 * @param address Pointer to the address structure.
 */
void bind_socket(int server_fd, struct sockaddr_in *address) {
    int opt = 1;
    address->sin_family = AF_INET;
    address->sin_addr.s_addr = INADDR_ANY;
    address->sin_port = htons(PORT);

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    if (bind(server_fd, (struct sockaddr *)address, sizeof(*address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
}

/**
 * Start listening for incoming connections on the server socket.
 *
 * @param server_fd File descriptor of the server socket.
 */
void start_listening(int server_fd) {
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
}

/**
 * Accept a connection on the server socket.
 *
 * @param server_fd File descriptor of the server socket.
 * @param address Pointer to the client address structure.
 * @return File descriptor of the accepted connection.
 */
int accept_connection(int server_fd, struct sockaddr_in *address) {
    int new_socket;
    int addrlen = sizeof(*address);

    if ((new_socket = accept(server_fd, (struct sockaddr *)address, (socklen_t *)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    return new_socket;
}

/**
 * Close the client and server sockets.
 *
 * @param new_socket File descriptor of the client socket.
 * @param server_fd File descriptor of the server socket.
 */
void close_sockets(int new_socket, int server_fd) {
    close(new_socket);
    close(server_fd);
    system("make clean_files");
}

/**
 * Receive a message from the socket.
 *
 * @param socket File descriptor of the socket.
 * @param msg Pointer to the message structure to store the received message.
 */
void receive_message(int socket, Message *msg) {
    // Initialize variables for tracking received bytes and total message length
    ssize_t total_bytes_received = 0;
    ssize_t bytes_received;

    // Loop until the entire message is received
    while (total_bytes_received < sizeof(Message)) {
        // Receive data into the message buffer starting from the last received position
        bytes_received = recv(socket, ((char *)msg) + total_bytes_received, sizeof(Message) - total_bytes_received, 0);

        if (bytes_received <= 0) {
            // Handle receive errors or connection closed
            perror("Error receiving message");
            exit(EXIT_FAILURE);
        }

        // Update the total received bytes
        total_bytes_received += bytes_received;
    }
}

/**
 * Handle the sender communication.
 *
 * @param client_socket File descriptor of the client socket.
 * @param file File pointer to write the received data.
 * @param times Pointer to the list to store transfer times.
 */
void sender_handler(int client_socket, FILE *file, List *times) {
    // Function implementation remains the same, just use the updated receive_message() function
    clock_t start, end;
    int i = 0;
    while (1) {
        Message msg;
        receive_message(client_socket, &msg);
        if (msg.type == FILE_DATA) {
            fwrite(msg.data, 1, msg.length, file);

        } else if (msg.type == CONTROL_MESSAGE) {
            if (strcmp(msg.data, "START") == 0) {
                start = clock();
            } else if (strcmp(msg.data, "END") == 0) {
                end = clock();
                insert(times, (((double)(end - start)) * 1000.0 / CLOCKS_PER_SEC));
                fclose(file);
                printf("File %d transfer completed\n", i + 1);
            } else if (strcmp(msg.data, "EXIT") == 0) {
                break;
            } else if (strcmp(msg.data, "SEND_AGAIN") == 0) {
                i++;
                char filename[50];
                snprintf(filename, 50, "assets/receive_file%d.txt", i);
                file = open_file_to_write(filename);
            }
        } else {
            perror("Error processing new data: ");
            fprintf(stdout, "Data failed processing - %ld bytes | DATA = %s\n", msg.length, msg.data);
            exit(EXIT_FAILURE);
        }
    }
}

/**
 * Main function.
 *
 * @param argc Number of command line arguments.
 * @param argv Array of command line arguments.
 * @return 0 indicating successful execution of the program.
 */
int main(int argc, char *argv[]) {

    int server_fd, client_socket;
    struct sockaddr_in address;

    List *times = createList();

    printf("Starting Receiver...\n");

    if (extract_Variables(argc, argv) == 1) {
        return 1;
    }
    create_server_socket(&server_fd);
    set_congestion_control(server_fd);

    bind_socket(server_fd, &address);
    creating_path();
    start_listening(server_fd);
    client_socket = accept_connection(server_fd, &address);

    printf("Sender connected, beginning to receive file...\n");
    FILE *file = open_file_to_write("assets/receive_file.txt");

    sender_handler(client_socket, file, times);

    print_times(size(times), times);
    printf("Receiver end..\n");
    deleteList(times);
    close_sockets(client_socket, server_fd);

    return 0;
}
