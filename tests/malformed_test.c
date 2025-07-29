#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

void send_raw_bytes(int sockfd, const char* data, int len, const char* description) {
    printf("Sending: %s\n", description);
    int sent = send(sockfd, data, len, 0);
    printf("  -> Sent %d bytes\n", sent);
    usleep(10000); // Small delay
}

void send_partial_message(int sockfd) {
    printf("\n=== Testing partial messages ===\n");
    
    // Send only 5 bytes of a 9-byte message
    char partial[5] = {'I', 0x00, 0x00, 0x30, 0x39};
    send_raw_bytes(sockfd, partial, 5, "Partial message (5 bytes)");
    
    // Send remaining 4 bytes later
    char remaining[4] = {0x00, 0x00, 0x00, 0x65};
    send_raw_bytes(sockfd, remaining, 4, "Remaining 4 bytes");
}

void send_invalid_messages(int sockfd) {
    printf("\n=== Testing invalid messages ===\n");
    
    // Invalid message type
    char invalid_type[9] = {'X', 0x00, 0x00, 0x30, 0x39, 0x00, 0x00, 0x00, 0x65};
    send_raw_bytes(sockfd, invalid_type, 9, "Invalid message type 'X'");
    
    // Empty message
    char empty[1] = {0};
    send_raw_bytes(sockfd, empty, 0, "Empty message (0 bytes)");
    
    // Too long message
    char too_long[15] = {'I', 0x00, 0x00, 0x30, 0x39, 0x00, 0x00, 0x00, 0x65, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    send_raw_bytes(sockfd, too_long, 15, "Too long message (15 bytes)");
    
    // Non-printable message type
    char non_printable[9] = {0xFF, 0x00, 0x00, 0x30, 0x39, 0x00, 0x00, 0x00, 0x65};
    send_raw_bytes(sockfd, non_printable, 9, "Non-printable message type");
}

void send_edge_case_values(int sockfd) {
    printf("\n=== Testing edge case values ===\n");
    
    // Valid insert with extreme values
    char max_values[9];
    max_values[0] = 'I';
    *(int32_t*)(max_values + 1) = htonl(2147483647);  // Max int32_t
    *(int32_t*)(max_values + 5) = htonl(2147483647);  // Max int32_t
    send_raw_bytes(sockfd, max_values, 9, "Insert with maximum int32_t values");
    
    // Valid insert with minimum values
    char min_values[9];
    min_values[0] = 'I';
    *(int32_t*)(min_values + 1) = htonl(-2147483648); // Min int32_t
    *(int32_t*)(min_values + 5) = htonl(-2147483648); // Min int32_t
    send_raw_bytes(sockfd, min_values, 9, "Insert with minimum int32_t values");
    
    // Query with swapped min/max (should return 0)
    char swapped_query[9];
    swapped_query[0] = 'Q';
    *(int32_t*)(swapped_query + 1) = htonl(1000);
    *(int32_t*)(swapped_query + 5) = htonl(500);  // max < min
    send_raw_bytes(sockfd, swapped_query, 9, "Query with mintime > maxtime");
    
    // Try to receive response
    int32_t response;
    int received = recv(sockfd, &response, 4, MSG_DONTWAIT);
    if (received == 4) {
        printf("  -> Received response: %d\n", ntohl(response));
    } else {
        printf("  -> No response received (expected for invalid query)\n");
    }
}

void send_rapid_fire(int sockfd) {
    printf("\n=== Testing rapid-fire messages ===\n");
    
    // Send 100 inserts as fast as possible
    for (int i = 0; i < 100; i++) {
        char message[9];
        message[0] = 'I';
        *(int32_t*)(message + 1) = htonl(90000 + i);
        *(int32_t*)(message + 5) = htonl(500 + i);
        send(sockfd, message, 9, 0);
    }
    printf("Sent 100 rapid-fire inserts\n");
    
    // Query the range
    char query[9];
    query[0] = 'Q';
    *(int32_t*)(query + 1) = htonl(90000);
    *(int32_t*)(query + 5) = htonl(90099);
    send_raw_bytes(sockfd, query, 9, "Query for all 100 inserts");
    
    int32_t response;
    if (recv(sockfd, &response, 4, 0) == 4) {
        printf("  -> Average of 100 values: %d\n", ntohl(response));
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <server_ip> <port>\n", argv[0]);
        printf("This test sends malformed and edge-case messages\n");
        return 1;
    }
    
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return 1;
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &server_addr.sin_addr);
    
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return 1;
    }
    
    printf("=== MALFORMED MESSAGE TEST ===\n");
    printf("Connected to server. Testing edge cases...\n");
    
    send_partial_message(sockfd);
    send_invalid_messages(sockfd);
    send_edge_case_values(sockfd);
    send_rapid_fire(sockfd);
    
    printf("\n=== TEST COMPLETED ===\n");
    printf("If your server is still running, it handled malformed messages well!\n");
    
    close(sockfd);
    return 0;
}
