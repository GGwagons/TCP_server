#include "../include/server.h"

/**
 * Signal handler for SIGINT (Ctrl+C) and SIGQUIT
 * Sets global flag to gracefully shutdown the server
 */
void sigHandler(int signum) {
	if (signum == SIGINT || signum == SIGQUIT) {
		g_signal = true;  								// Set flag to exit main loop
	}
}

/**
 * Error handling function that cleans up and exits
 * Closes server socket and writes error message to stderr
 */
void exiterror(const char *msg) {
	if (server > 2) close(server);  					// Close server socket if it's open (fd > 2)
	for (size_t i = 0; msg[i] != '\0'; ++i) {			//	putstr but specified fd (stderr = 2)
		write (2, &msg[i], 1);
	}
	exit(1);
}

/**
 * Initialize client session data when a new client connects
 * Allocates memory for price storage and sets initial values
 * Returns: 0 on success, -1 on memory allocation failure
 */
int init_client_data(int fd) {
	// Allocate initial memory for 100 price entries
	client_sessions[fd].prices = malloc(sizeof(price_entry_t) * 100);
	if (!client_sessions[fd].prices) {
		return -1;
	}
	// Initialize session state
	client_sessions[fd].count = 0;       				// No prices stored yet
	client_sessions[fd].capacity = 100;  				// Can hold 100 prices initially
	return 0;  // Success
}

/**
 * Clean up client session data when client disconnects
 * Frees allocated memory and resets all values to prevent memory leaks
 */
void cleanup_client_data(int fd) {
	if (client_sessions[fd].prices) {
		free(client_sessions[fd].prices);     			// Free the price array
		client_sessions[fd].prices = NULL;    			// Prevent double-free
		client_sessions[fd].count = 0;        			// Reset count
		client_sessions[fd].capacity = 0;     			// Reset capacity  
	}
}

/**
 * Insert a price entry for a client, maintaining chronological order
 * Handles dynamic memory allocation if more space is needed
 * Prices are kept sorted by timestamp for efficient querying
 */
void insert_price(int fd, int32_t timestamp, int32_t price) {
	int				i;
	client_data_t	*session;
	session = &client_sessions[fd];  					// Get client's session data
	
	if (session->count >= session->capacity) {			// Check if more memory is needed (array is full)
		session->capacity *= 2;  						// Double the capacity
		// Reallocate memory for more price entries
		price_entry_t *new_prices = realloc(session->prices, sizeof(price_entry_t) * session->capacity);
		if (!new_prices) {
			return; 									// Memory allocation failed - keep old array intact, don't add this price
		}
		session->prices = new_prices;  					// Update pointer to new memory - realloc frees the old data
	}
	
	// Find correct position to insert (maintain chronological order) - start from the end && shift entries right
	for (i = session->count; i > 0 && session->prices[i - 1].timestamp > timestamp; --i) {
		session->prices[i] = session->prices[i - 1];
	}
	session->prices[i].timestamp = timestamp; 			// Insert the new price entry at position i
	session->prices[i].price = price;
	session->count++;
}

/**
 * Calculate average price for a client within a time range
 * Returns the mean price of all entries with timestamps in [mintime, maxtime]
 * Returns 0 if no prices found in range or if range is invalid
 */
int32_t query_average_price(int fd, int32_t mintime, int32_t maxtime) {
	client_data_t	*session = &client_sessions[fd]; 	// Get client's session data
	long long		sum = 0;    						// Use long long to prevent overflow with large sums
	int				count = 0;		  					// Count of prices found in the time range

	if (mintime > maxtime) {							// Check for invalid time range (as per requirements)
		return 0;
	}
	// Iterate through all stored prices for this client
	for (size_t i = 0; i < session->count; i++) {
		// Check if this price's timestamp falls within the requested range
		if (session->prices[i].timestamp >= mintime && session->prices[i].timestamp <= maxtime) {
			sum += session->prices[i].price;  			// Add price to running total
			count++;
		}
	}
	if (count == 0) return 0;							// If no prices found in range, return 0 (as per requirements)
	return (int32_t)(sum / count);						// Calculate and return average (integer division, truncates decimals)
}

/**
 * Process a complete 9-byte message from a client
 * Parses the binary message and performs the requested operation (Insert or Query)
 * Message format: 1 byte type + 2×4-byte integers in network byte order
 */
void handle_message(int fd) {
	char	msg_type = buff[0];                          	// First byte: 'I' or 'Q'
	int32_t	first_int = ntohl(*(int32_t*)(buff + 1)); 		// Bytes 1-4: first integer
	int32_t	second_int = ntohl(*(int32_t*)(buff + 5));		// Bytes 5-8: second integer
	
	if (msg_type == 'I') {
		insert_price(fd, first_int, second_int);			// Insert operation: first_int = timestamp, second_int = price
	}
	else if (msg_type == 'Q') {								// Query operation: first_int = mintime, second_int = maxtime
		int32_t average = query_average_price(fd, first_int, second_int);
		int32_t response = htonl(average);  				// Convert to network byte order
		send(fd, &response, RESPONSE_SIZE, 0);				// Send 4-byte response back to client
	}														// Invalid message types are ignored (undefined behavior allowed per spec)
}

/** 
 * Checks if AV[1] has only digits
 * Converts from string to int
 * Checks is port is in range 1024-65535
*/
int	checkPort(char *av) {
	for (size_t i = 0; i < strlen(av); ++i) {
		if (isdigit(av[i])) {
			continue;
		}
		return -1;
	}
	int port = atoi(av);
	if (port < 1024 || port > 65535) {
		return (-1);
	}
	return (port);
}

/**
 * Create and configure the TCP server socket
 * Sets up socket address, binds to port, and starts listening for connections
 * Initializes the file descriptor set for select() monitoring
 */
void server_create(char **av) {
	int port = checkPort(av[1]);							// Port validation
	if (port <= 0){
		exiterror("Valid port range 1024 - 65535\n");		// 0-1023 reserved for the big bois
	}
	
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;               			// IPv4
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);			// Listen on all interfaces
	servaddr.sin_port = htons(port);      					// Port from command line
	server = socket(AF_INET, SOCK_STREAM, 0);				// Create TCP socket
	
	if (server < 0) {
		exiterror("Socket creation failed\n");
	}
	if ((bind(server, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) {	// Bind socket to address and port
		exiterror("Bind failed (port might be in use)\n");
	}	
	if (listen(server, 10) != 0) { 							// Start listening for connections (queue up to 10 pending connections)
		exiterror("Listen failed\n");
	}
	// Initialize file descriptor set for select()
	FD_ZERO(&requests);           							// Clear the set
	FD_SET(server, &requests);    							// Add server socket to monitoring set
	last = server;                							// Track highest file descriptor number
}

/**
 * Main server event loop - handles client connections and messages
 * Uses select() for non-blocking I/O to handle multiple clients simultaneously
 * Continues until a signal (SIGINT/SIGQUIT) is received
 */
void main_loop() {
	while (!g_signal) {
		readtime = requests; 								// Copy the file descriptor set (select() modifies it)
		if (select(last + 1, &readtime, NULL, NULL, NULL) < 0) {
			continue;
		}
		if (FD_ISSET(server, &readtime)) { 					// Check if the server socket has a new connection waiting
			client = accept(server, (struct sockaddr *)&cli, &len);
			if (client < 0) {
				exiterror(" Accept failed - critical error\n");
			}
			// Ensure client FD doesn't exceed our array size && initialize client session data
			if (client >= 1024 || init_client_data(client) != 0) {
				close(client);  							// Cannot handle this client - reject connection
				continue;       							// Skip to next iteration
			}
			FD_SET(client, &requests); 						// Add new client to monitoring set
			last = last > client ? last : client;			// Update highest file descriptor number for select()
			continue;  										// Process this new connection on next iteration
		}
		for (int fd = 3; fd <= last; ++fd) {				// Check all possible client file descriptors for incoming data
			if (!FD_ISSET(fd, &readtime)) continue; 		// Skip if this FD doesn't have data ready
			bzero(buff, MSG_SIZE);							// Clear buffer before receiving new data
			r = recv(fd, buff, MSG_SIZE, 0);				// Try to receive exactly MSG_SIZE (9) bytes from client
			if (r <= 0) {									// Handle client disconnection or error
				cleanup_client_data(fd);    				// Free client's price data
				FD_CLR(fd, &requests);		      			// Remove from monitoring set
				close(fd);
			}
			else if (r == MSG_SIZE) {						// Handle complete message received
				handle_message(fd);							// Got exactly 9 bytes - process the complete message
			}	
		}													// Partial messages (r > 0 && r < MSG_SIZE) are ignored - acceptable per the protocol specification
	}														// Individual client cleanup happens automatically when process exits
	close(server); 											// Close server socket to stop accepting new connections
}

/**
 * Main function - entry point of the TCP price server
 * Validates command line arguments, sets up signal handling, creates server, and starts main loop
 */
int main(int ac, char **av) {
	if (ac != 2) {
		exiterror("Expected usage: ./price_server <port_number>\n");
	}
	signal(SIGINT, sigHandler);   							// Set up signal handlers for graceful shutdown
	signal(SIGQUIT, sigHandler);
	server_create(av);										// Create and configure the TCP server socket
	main_loop();											// Start the main event loop
	return (0);
}
