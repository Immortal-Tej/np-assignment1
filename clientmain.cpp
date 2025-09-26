
// This is a TCP client program that connects to a server
// It talks to the server and does math calculations


#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

// I'm including this library for the calculator functions
#include <calcLib.h>

using namespace std;

int main(int argc, char *argv[]) {
    // Check if the user provided the right arguments
    if (argc != 2) {
        cout << "Usage: ./client <host:port>" << endl;
        return 1;
    }
    
    
    // Parse the host and port from the command line
    string input = argv[1];
    
    string hostname;
    string port_string;
    
    
    if (input[0] == '[') {
        size_t bracket_end = input.find(']');
        if (bracket_end == string::npos || bracket_end + 1 >= input.length() || input[bracket_end + 1] != ':') {
            cout << "Invalid bracketed IPv6 format" << endl;
            return 1;
        }
        hostname = input.substr(1, bracket_end - 1);  // Extract between brackets
        port_string = input.substr(bracket_end + 2);  // After ]:
    } else {
        // Find the last colon to separate host and port
        // This works for IPv4 (127.0.0.1:5000) and simple IPv6 (::1:5000)
        size_t colon_pos = input.rfind(':');
        if (colon_pos == string::npos) {
            cout << "Invalid host:port format" << endl;
            return 1;
        }
        
        hostname = input.substr(0, colon_pos);
        port_string = input.substr(colon_pos + 1);
    }
    
    cout << "Host " << hostname << ", and port " << port_string << "." << endl;
    
    
    // Get address information for both IPv4 and IPv6
    // This will help us connect to any type of address
    struct addrinfo hints;
    struct addrinfo *result;
    
    // Set up hints for what kind of address we want
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;    // Allow both IPv4 and IPv6
    hints.ai_socktype = SOCK_STREAM; // We want TCP connection
    
    // Try to get address info for the hostname and port
    int addr_result = getaddrinfo(hostname.c_str(), port_string.c_str(), &hints, &result);
    
    if (addr_result != 0) {
        cout << "ERROR: RESOLVE ISSUE" << endl;
        return 1;
    }
    
    
    // Try to connect using the resolved addresses
    // We'll try each address until one works
    int client_socket = -1;
    struct addrinfo *current_addr;
    
    for (current_addr = result; current_addr != NULL; current_addr = current_addr->ai_next) {
        
        // Create a socket for this address type
        client_socket = socket(current_addr->ai_family, current_addr->ai_socktype, current_addr->ai_protocol);
        
        if (client_socket < 0) {
            continue; // Try next address if socket creation fails
        }
        
        // Try to connect using this address
        if (connect(client_socket, current_addr->ai_addr, current_addr->ai_addrlen) == 0) {
            break; // Connection successful!
        }
        
        // Connection failed, close socket and try next address
        close(client_socket);
        client_socket = -1;
    }
    
    // Clean up the address info memory
    freeaddrinfo(result);
    
    // Check if we managed to connect
    if (client_socket < 0) {
        cout << "ERROR: CANT CONNECT TO " << hostname << endl;
        return 1;
    }
    
    
    // Set a timeout for reading so we don't hang forever
    struct timeval timeout;
    timeout.tv_sec = 5;  // 5 second timeout
    timeout.tv_usec = 0;
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    
    // Read the first line from the server
    string first_line = "";
    
    char c;
    
    // Read one character at a time until we get a newline
    while (true) {
        int bytes_read = recv(client_socket, &c, 1, 0);
        
        if (bytes_read <= 0) {
            cout << "ERROR: MISSMATCH PROTOCOL" << endl;
            close(client_socket);
            return 1;
        }
        
        if (c == '\n') {
            break;
        }
        
        first_line = first_line + c;
    }
    
    
    // Check if the first line is exactly what we need
    // We only support "TEXT TCP 1.0"
    if (first_line != "TEXT TCP 1.0") {
        cout << "ERROR: MISSMATCH PROTOCOL" << endl;
        close(client_socket);
        return 1;
    }
    
    
    // Now read the rest of the protocol lines until empty line
    // But limit how many lines we read to avoid hanging
    int max_protocol_lines = 10;
    int lines_read = 0;
    
    
    while (lines_read < max_protocol_lines) {
        string protocol_line = "";
        
        // Read one character at a time until we get a newline
        while (true) {
            int bytes_read = recv(client_socket, &c, 1, 0);
            
            if (bytes_read <= 0) {
                // Timeout or connection error, but we already got TEXT TCP 1.0
                break;
            }
            
            if (c == '\n') {
                break;
            }
            
            protocol_line = protocol_line + c;
        }
        
        // If we get an empty line, we're done reading protocols
        if (protocol_line.empty()) {
            break;
        }
        
        lines_read++;
    }
    
    
    // Send OK to the server
    string ok_message = "OK\n";
    
    int send_result = send(client_socket, ok_message.c_str(), ok_message.length(), 0);
    
    if (send_result <= 0) {
        cout << "Failed to send OK message" << endl;
        close(client_socket);
        return 1;
    }
    
    
    // Read the assignment from the server
    string assignment_line = "";
    
    while (true) {
        int bytes_read = recv(client_socket, &c, 1, 0);
        
        if (bytes_read <= 0) {
            cout << "Failed to read assignment" << endl;
            close(client_socket);
            return 1;
        }
        
        
        if (c == '\n') {
            break;
        }
        
        assignment_line = assignment_line + c;
    }
    
    
    // Parse the assignment line to get operation and values
    string operation;
    string value1_string;
    string value2_string;
    
    
    // Find the first space
    int space1 = assignment_line.find(' ');
    
    if (space1 == string::npos) {
        cout << "Invalid assignment format" << endl;
        close(client_socket);
        return 1;
    }
    
    
    // Find the second space
    int space2 = assignment_line.find(' ', space1 + 1);
    
    if (space2 == string::npos) {
        cout << "Invalid assignment format" << endl;
        close(client_socket);
        return 1;
    }
    
    
    operation = assignment_line.substr(0, space1);
    
    value1_string = assignment_line.substr(space1 + 1, space2 - space1 - 1);
    
    value2_string = assignment_line.substr(space2 + 1);
    
    
    cout << "ASSIGNMENT: " << operation << " " << value1_string << " " << value2_string << endl;
    
    
    
    // Calculate the result based on the operation
    string result_string;
    
    
    if (operation[0] == 'f') {
        
        // This is a floating point operation
        double value1 = atof(value1_string.c_str());
        double value2 = atof(value2_string.c_str());
        
        double result;
        
        
        if (operation == "fadd") {
            result = value1 + value2; // Addition
            
        } else if (operation == "fsub") {
            result = value1 - value2; // Subtraction
            
        } else if (operation == "fmul") {
            result = value1 * value2; // Multiplication
            
        } else if (operation == "fdiv") {
            result = value1 / value2; // Division
            
        } else {
            result = 0.0; // Default case for unknown operation
        }
        
        
        // Formating the floating point
        char buffer[100];
        sprintf(buffer, "%8.8g", result);
        
        result_string = string(buffer) + "\n";
        
    } else {
        
        // This is an integer operation
        long long value1 = atoll(value1_string.c_str());
        long long value2 = atoll(value2_string.c_str());
        
        long long result;
        
        
        if (operation == "add") {
            result = value1 + value2; // Addition
            
        } else if (operation == "sub") {
            result = value1 - value2; // Subtraction
            
        } else if (operation == "mul") {
            result = value1 * value2; // Multiplication
            
        } else if (operation == "div") {
            result = value1 / value2; // Integer division
            
        } else {
            result = 0; // Default case for unknown operation
        }
        
        
        // Convert result to string
        char buffer[100];
        sprintf(buffer, "%lld", result);
        
        result_string = string(buffer) + "\n";
        
    }
    
    
    // Send the result back to the server
    int send_result2 = send(client_socket, result_string.c_str(), result_string.length(), 0);
    
    if (send_result2 <= 0) {
        cout << "Failed to send result" << endl;
        close(client_socket);
        return 1;
    }
    
    
    // Read the server's response (OK or ERROR)
    string server_response = "";
    
    
    while (true) {
        int bytes_read = recv(client_socket, &c, 1, 0);
        
        if (bytes_read <= 0) {
            cout << "Failed to read server response" << endl;
            close(client_socket);
            return 1;
        }
        
        
        if (c == '\n') {
            break;
        }
        
        server_response = server_response + c;
    }
    
    
    // Remove the newline from result_string for display
    string display_result = result_string;
    
    if (!display_result.empty() && display_result.back() == '\n') {
        display_result.pop_back();
    }
    
    
    // Print the final result
    cout << server_response << " (myresult=" << display_result << ")" << endl;
    
    
    close(client_socket);
    
    
    return 0; // Success!
}