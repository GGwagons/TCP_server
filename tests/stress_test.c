#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>

void send_insert(int sockfd, int32_t timestamp, int32_t price) {
    char message[9];
    message[0] = 'I';
    *(int32_t*)(message + 1) = htonl(timestamp);
    *(int32_t*)(message + 5) = htonl(price);
    
    if (send(sockfd, message, 9, 0) != 9) {
        perror("Failed to send insert message");
    }
}

int32_t send_query(int sockfd, int32_t mintime, int32_t maxtime) {
    char message[9];
    message[0] = 'Q';
    *(int32_t*)(message + 1) = htonl(mintime);
    *(int32_t*)(message + 5) = htonl(maxtime);
    
    if (send(sockfd, message, 9, 0) != 9) {
        perror("Failed to send query message");
        return -1;
    }
    
    int32_t response;
    if (recv(sockfd, &response, 4, 0) != 4) {
        perror("Failed to receive query response");
        return -1;
    }
    
    return ntohl(response);
}

void client_worker(int client_id, const char* server_ip, int port) {
    printf("Client %d: Starting...\n", client_id);
    
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("Client %d: Socket creation failed\n", client_id);
        return;
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);
    
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Client %d: Connection failed\n", client_id);
        close(sockfd);
        return;
    }
    
    printf("Client %d: Connected successfully\n", client_id);
    
    // Each client inserts data in its own timestamp range
    int base_time = client_id * 10000;
    
    // Insert 50 price entries per client
    for (int i = 0; i < 50; i++) {
        send_insert(sockfd, base_time + i, 100 + (i % 20));
        usleep(1000); // Small delay to simulate real usage
    }
    
    // Perform some queries
    int32_t result1 = send_query(sockfd, base_time, base_time + 25);
    int32_t result2 = send_query(sockfd, base_time + 25, base_time + 49);
    int32_t result3 = send_query(sockfd, base_time, base_time + 49);
    
    printf("Client %d: Query results: %d, %d, %d\n", client_id, result1, result2, result3);
    
    // Test cross-client queries (should return 0 since data is isolated)
    int other_base = ((client_id + 1) % 10) * 10000;
    int32_t cross_result = send_query(sockfd, other_base, other_base + 49);
    printf("Client %d: Cross-client query result: %d (should be 0)\n", client_id, cross_result);
    
    printf("Client %d: Finished\n", client_id);
    close(sockfd);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <server_ip> <port>\n", argv[0]);
        printf("This stress test creates 10 concurrent clients\n");
        return 1;
    }
    
    const char* server_ip = argv[1];
    int port = atoi(argv[2]);
    int num_clients = 10;
    
    printf("=== MULTI-CLIENT STRESS TEST ===\n");
    printf("Creating %d concurrent clients...\n\n", num_clients);
    
    time_t start_time = time(NULL);
    
    // Fork multiple client processes
    for (int i = 0; i < num_clients; i++) {
        pid_t pid = fork();
        
        if (pid == 0) {
            // Child process - run client
            client_worker(i, server_ip, port);
            exit(0);
        } else if (pid < 0) {
            perror("Fork failed");
            return 1;
        }
        
        // Small delay between client starts
        usleep(100000); // 100ms
    }
    
    // Parent process - wait for all children
    printf("Parent: Waiting for all clients to complete...\n");
    for (int i = 0; i < num_clients; i++) {
        int status;
        wait(&status);
    }
    
    time_t end_time = time(NULL);
    printf("\n=== STRESS TEST COMPLETED ===\n");
    printf("All %d clients finished in %ld seconds\n", num_clients, end_time - start_time);
    printf("If your server handled this without crashes or memory leaks, it's robust!\n");
    
    return 0;
}
