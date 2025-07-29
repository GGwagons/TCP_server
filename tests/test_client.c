#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

void send_insert(int sockfd, int32_t timestamp, int32_t price) {
    char message[9];
    message[0] = 'I';
    *(int32_t*)(message + 1) = htonl(timestamp);
    *(int32_t*)(message + 5) = htonl(price);
    
    if (send(sockfd, message, 9, 0) != 9) {
        perror("Failed to send insert message");
    } else {
        printf("Sent: Insert timestamp=%d, price=%d\n", timestamp, price);
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
    
    printf("Sent: Query mintime = %d, maxtime = %d\n", mintime, maxtime);
    
    int32_t response;
    if (recv(sockfd, &response, 4, 0) != 4) {
        perror("Failed to receive query response");
        return -1;
    }
    
    int32_t average = ntohl(response);
    printf("Received: Average price = %d\n", average);
    return average;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <server_ip> <port>\n", argv[0]);
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
    
    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        return 1;
    }
    
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return 1;
    }
    
    printf("Connected to server %s:%s\n", argv[1], argv[2]);
    printf("Running comprehensive test suite...\n\n");
    
    // === TEST 1: Basic functionality (from info.txt) ===
    printf("=== TEST 1: Basic functionality ===\n");
    send_insert(sockfd, 12345, 101);
    send_insert(sockfd, 12346, 102);
    send_insert(sockfd, 12347, 100);
    send_insert(sockfd, 40960, 5);
    
    send_query(sockfd, 12288, 16384);  // Should return 101 (average of 101,102,100)
    send_query(sockfd, 40000, 50000);  // Should return 5 (only one price in range)
    send_query(sockfd, 10000, 12000);  // Should return 0 (no prices in range)
    send_query(sockfd, 16384, 12288);  // Should return 0 (mintime > maxtime)
    
    // === TEST 2: Edge cases with extreme values ===
    printf("\n=== TEST 2: Extreme values ===\n");
    send_insert(sockfd, 0, 1);              // Minimum timestamp
    send_insert(sockfd, 2147483647, 999);   // Maximum positive int32_t
    send_insert(sockfd, -2147483648, -500); // Minimum negative int32_t
    send_insert(sockfd, -1, 0);             // Negative timestamp, zero price
    
    send_query(sockfd, -2147483648, 2147483647); // Full range query
    send_query(sockfd, 0, 0);                    // Single timestamp query
    send_query(sockfd, -1, -1);                  // Query negative timestamp
    
    // === TEST 3: Duplicate timestamps ===
    printf("\n=== TEST 3: Duplicate timestamps ===\n");
    send_insert(sockfd, 1000, 50);
    send_insert(sockfd, 1000, 60);  // Same timestamp, different price
    send_insert(sockfd, 1000, 70);  // Same timestamp again
    
    send_query(sockfd, 1000, 1000); // Should return average of 50,60,70 = 60
    
    // === TEST 4: Large dataset (test memory allocation) ===
    printf("\n=== TEST 4: Large dataset (200 entries) ===\n");
    printf("Inserting 200 price entries...\n");
    for (int i = 0; i < 200; i++) {
        send_insert(sockfd, 50000 + i, 100 + (i % 50)); // Prices 100-149
    }
    
    send_query(sockfd, 50000, 50099);   // First 100 entries
    send_query(sockfd, 50100, 50199);   // Last 100 entries
    send_query(sockfd, 50050, 50149);   // Middle 100 entries
    
    // === TEST 5: Out-of-order insertions ===
    printf("\n=== TEST 5: Out-of-order insertions ===\n");
    send_insert(sockfd, 30000, 300);
    send_insert(sockfd, 20000, 200);  // Earlier timestamp
    send_insert(sockfd, 25000, 250);  // Middle timestamp
    send_insert(sockfd, 35000, 350);  // Later timestamp
    send_insert(sockfd, 22000, 220);  // Another early one
    
    send_query(sockfd, 19000, 36000); // Should find all 5 and average them
    send_query(sockfd, 21000, 26000); // Should find 200, 220, 250
    
    // === TEST 6: Boundary conditions ===
    printf("\n=== TEST 6: Boundary conditions ===\n");
    send_insert(sockfd, 60000, 600);
    send_insert(sockfd, 60001, 601);
    send_insert(sockfd, 60002, 602);
    
    send_query(sockfd, 60000, 60000); // Exact match
    send_query(sockfd, 60000, 60001); // Boundary inclusive
    send_query(sockfd, 59999, 60003); // Wider range
    send_query(sockfd, 60003, 60010); // No matches
    
    // === TEST 7: Zero and negative prices ===
    printf("\n=== TEST 7: Zero and negative prices ===\n");
    send_insert(sockfd, 70000, 0);    // Zero price
    send_insert(sockfd, 70001, -100); // Negative price
    send_insert(sockfd, 70002, -200); // Another negative
    send_insert(sockfd, 70003, 100);  // Positive for balance
    
    send_query(sockfd, 70000, 70003); // Mix of zero, negative, positive
    send_query(sockfd, 70000, 70002); // Just zero and negatives
    
    // === TEST 8: Single entry queries ===
    printf("\n=== TEST 8: Single entry scenarios ===\n");
    send_insert(sockfd, 80000, 800);
    send_query(sockfd, 80000, 80000); // Query single entry
    send_query(sockfd, 79999, 79999); // Query non-existent before
    send_query(sockfd, 80001, 80001); // Query non-existent after
    
    // === TEST 9: Query without any inserts (should be 0) ===
    printf("\n=== TEST 9: Empty range queries ===\n");
    send_query(sockfd, 90000, 90100); // Range with no data
    send_query(sockfd, -10000, -9000); // Negative range with no data
    
    // === TEST 10: Stress test - rapid alternating inserts/queries ===
    printf("\n=== TEST 10: Rapid insert/query alternation ===\n");
    for (int i = 0; i < 10; i++) {
        send_insert(sockfd, 100000 + i, 1000 + i);
        send_query(sockfd, 100000, 100000 + i); // Query growing range
    }
    
    printf("\n=== All tests completed! ===\n");
    
    close(sockfd);
    return 0;
}
