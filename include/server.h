#ifndef SERVER_H
#	define SERVER_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <stdbool.h>
#include <signal.h> 
#include <arpa/inet.h>
#include <ctype.h>

#define MSG_SIZE        9                           // Size of client messages (1 byte type + 2×4 byte integers)
#define RESPONSE_SIZE   4                           // Size of server response (4 byte integer)

/**
 * Structure representing a single price entry
 * Each entry contains a timestamp and corresponding price value
 */
typedef struct {
    int32_t             timestamp;                  // When the price was recorded
    int32_t             price;                      // The price value at that time

} price_entry_t;

/**
 * Structure representing a client's session data
 * Each connected client has their own isolated data storage
 */
typedef struct {
    price_entry_t       *prices;                    // Dynamic array of price entries
    size_t              count;                      // Current number of stored prices
    size_t              capacity;                   // Maximum number of prices array can hold

} client_data_t;


bool                    g_signal = false;           // Flag set by signal handler to trigger shutdown
static int32_t          server, client, last;       // server: listening socket, client: current client, last: highest FD number
static client_data_t    client_sessions[1024];      // Per-client data storage (indexed by file descriptor)
static char             buff[MSG_SIZE];             // Buffer for incoming 9-byte messages
fd_set                  requests, readtime;         // requests: master FD set, readtime: working copy for select()
struct sockaddr_in      servaddr, cli;              // servaddr: server address, cli: client address
socklen_t               len = sizeof(cli);          // Size of client address structure
ssize_t                 r;                          // Return value from recv() calls

#endif