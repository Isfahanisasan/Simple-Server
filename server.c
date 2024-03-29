#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h> 
#include <ctype.h>

/**
 * All parts needed to be changed/added are marked with TODO
 */

#define BUFFER_SIZE 1024
#define DEFAULT_SERVER_PORT 8081
#define DEFAULT_REMOTE_HOST "131.179.176.34"
#define DEFAULT_REMOTE_PORT 5001

struct server_app {
    // Parameters of the server
    // Local port of HTTP server
    uint16_t server_port;

    // Remote host and port of remote proxy
    char *remote_host;
    uint16_t remote_port;
};




void decode_path(const char *input, char *output) {
    while (*input) {
        if (*input == '%' && isxdigit(*(input + 1)) && isxdigit(*(input + 2))) {
            // Convert the next two hex digits to a character
            char hex[3] = {input[1], input[2], '\0'};
            *output++ = strtol(hex, NULL, 16);
            input += 3;  // Skip past the "%XX"
        } else if (*input == '%') {
            // Literal '%' character, escape it or handle as needed
            *output++ = '%';  // Copy '%' literally
            input++;
        } else {
            // Regular character, copy it
            *output++ = *input++;
        }
    }
    *output = '\0';  // Null-terminate the output string
}



// The following function is implemented for you and doesn't need
// to be change
void parse_args(int argc, char *argv[], struct server_app *app);

// The following functions need to be updated
void handle_request(struct server_app *app, int client_socket);
void serve_local_file(int client_socket, const char *path);
void proxy_remote_file(struct server_app *app, int client_socket, const char *path);

// The main function is provided and no change is needed
int main(int argc, char *argv[])
{
    struct server_app app;
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    int ret;

    parse_args(argc, argv, &app);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(app.server_port);

    // The following allows the program to immediately bind to the port in case
    // previous run exits recently
    int optval = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 10) == -1) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", app.server_port);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket == -1) {
            perror("accept failed");
            continue;
        }
        
        printf("Accepted connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        handle_request(&app, client_socket);
        close(client_socket);
    }

    close(server_socket);
    return 0;
}

void parse_args(int argc, char *argv[], struct server_app *app)
{
    int opt;

    app->server_port = DEFAULT_SERVER_PORT;
    app->remote_host = NULL;
    app->remote_port = DEFAULT_REMOTE_PORT;

    while ((opt = getopt(argc, argv, "b:r:p:")) != -1) {
        switch (opt) {
        case 'b':
            app->server_port = atoi(optarg);
            break;
        case 'r':
            app->remote_host = strdup(optarg);
            break;
        case 'p':
            app->remote_port = atoi(optarg);
            break;
        default: /* Unrecognized parameter or "-?" */
            fprintf(stderr, "Usage: server [-b local_port] [-r remote_host] [-p remote_port]\n");
            exit(-1);
            break;
        }
    }

    if (app->remote_host == NULL) {
        app->remote_host = strdup(DEFAULT_REMOTE_HOST);
    }
}

void handle_request(struct server_app *app, int client_socket) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    // Read the request from HTTP client
    // Note: This code is not ideal in the real world because it
    // assumes that the request header is small enough and can be read
    // once as a whole.
    // However, the current version suffices for our testing.
    bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) {
        return;  // Connection closed or error
    }

    buffer[bytes_read] = '\0';//when data is received from recv it is not automatically 
    //null terminated, so you need to null terminate it to make it C-style string
    // copy buffer to a new string
    // char *request = malloc(strlen(buffer) + 1);//buffer needs to be saved for later operations
    // strcpy(request, buffer);

    // TODO: Parse the header and extract essential fields, e.g. file name
    char method[10], path[1024], protocol[10]; 
    sscanf(buffer, "%s %s %s", method, path, protocol);
    // Hint: if the requested path is "/" (root), default to index.html
    // printf(request);
    char file_name[1024];
    if (strcmp(path, "/") == 0){//strcmpt returns 0 if two c-strings are equal 
        strcpy(file_name, "index.html"); 
    } else { 
        // strcpy(file_name, path + 1); //+1 to skip the leading / 
        decode_path(path + 1, file_name); 
    }
    
    int fileLength = strlen(file_name);
    // for(int i = 0; i < fileLength - 2; i++){ // converting %20 into space to match files with spaces
    //     if (file_name[i] == '%' && file_name[i + 1] == '2' && file_name[i + 2] == '0'){
    //         file_name[i] = ' ';
    //         memmove(&file_name[i + 1], &file_name[i + 3], fileLength - i - 2);
    //         fileLength -= 2; 
    //     }
    // }
    // TODO: Implement proxy and call the function under condition
    // specified in the spec
    // if (need_proxy(...)) {
    //    proxy_remote_file(app, client_socket, file_name);
    // } else {
    printf("checkpoint \n");
    printf("File name: %s", file_name);
    
    if(strstr(file_name, ".ts") != NULL){

        char request_copy[BUFFER_SIZE]; // Create a local copy of the buffer
        strncpy(request_copy, buffer, sizeof(request_copy));
        printf("\ncheckpoint 22 \n");
        printf(request_copy);
        proxy_remote_file(app, client_socket, request_copy); // Pass the copy to proxy_remote_file

    } else {
        serve_local_file(client_socket, file_name);
    }
}

void serve_local_file(int client_socket, const char *file_name) {
    // TODO: Properly implement serving of local files
    // The following code returns a dummy response for all requests
    // but it should give you a rough idea about what a proper response looks like
    // What you need to do 
    // (when the requested file exists):
    // * Open the requested file
    // * Build proper response headers (see details in the spec), and send them
    // * Also send file content
    // (When the requested file does not exist):
    // * Generate a correct response
    // printf("File name: %s\n", file_name);
    FILE *file = fopen(file_name, "rb");
    if (file == NULL){
        printf("File is NULL\n");
        char *notFoundResponse = "HTTP/1.0 404 Not Found\r\n\r\n";

        send(client_socket, notFoundResponse, strlen(notFoundResponse), 0);
        return;
    }

    char *contentType;

    if (strstr(file_name, ".txt") != NULL) { //first occurance strstr
    printf("File is .txt\n");
        contentType = "text/plain; charset=UTF-8";
    }
    else if (strstr(file_name, ".html") != NULL) {
        contentType = "text/html; charset=UTF-8";
    } else if (strstr(file_name, ".jpg") != NULL) {
        contentType = "image/jpeg";
    } else {
        contentType = "application/octet-stream";
    }
    //Get file size
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file); 
    fseek(file, 0, SEEK_SET); 

    //build the response
    char response[1024];
    sprintf(response, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n", contentType, fileSize);

    // printf(response);
    send(client_socket, response, strlen(response), 0);

    // Read and send the file content
    char buffer[BUFFER_SIZE];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        send(client_socket, buffer, bytesRead, 0);
    }
    fclose(file);
}

void proxy_remote_file(struct server_app *app, int client_socket, const char *request) {
    // TODO: Implement proxy request and replace the following code
    // What's needed:
    // * Connect to remote server (app->remote_server/app->remote_port)
    // * Forward the original request to the remote server
    // * Pass the response from remote server back
    // Bonus:
    // * When connection to the remote server fail, properly generate
    // HTTP 502 "Bad Gateway" response
    printf("checkpoint 4 \n");
    int local_socket; 
    char buffer[BUFFER_SIZE]; 
    ssize_t bytes_read, bytes_sent; 

    
    local_socket = socket(AF_INET, SOCK_STREAM, 0); //ipv4 is AF_INET
    if(local_socket < 0){
        perror("socket failed"); 
        char response[] = "HTTP/1.0 502 Bad Gateway\r\n\r\n";
        send(client_socket, response, strlen(response), 0); 
        return; 
    }
    printf("checkpoint 5 \n");

    
    struct sockaddr_in local_addr; 
    // //prepare remote server address structure 
    memset(&local_addr, 0, sizeof(local_addr)); //zero out the remote 
    local_addr.sin_family = AF_INET; 
    local_addr.sin_port= htons(app->remote_port);
    

    if (inet_pton(AF_INET, "127.0.0.1", &local_addr.sin_addr) <= 0) { //sets remote host
        perror("Invalid address/ Address not supported");
        close(local_socket);
        return;
    }

    // printf("checkpoint 3 \n");

    // // printf("Remote Host: %s\n", app->remote_host);
    // printf("Remote_addr Sin Port: %hu\n", ntohs(local_addr.sin_port)); // Use %hu to format the port
    // printf("Remote Server IP Address: %s\n", inet_ntoa(local_addr.sin_addr));
    // printf("checkpoint 4\n");


    if(connect(local_socket, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0){
        perror("connect failed 22");
        // fprintf(stderr, "connect error: %s\n", strerror(errno));

        close(local_socket);
        const char *bad_gateway_response = "HTTP/1.1 502 Bad Gateway\r\n\r\n";
        send(client_socket, bad_gateway_response, strlen(bad_gateway_response), 0);
        return;  
    }
    // printf("checkpoint");

    // // Forward the client's request to the backend video serve
    bytes_sent = send(local_socket, request, strlen(request), 0); 
    if(bytes_sent < 0){
        perror("send to remote server failed"); 
        close(local_socket);
        return;
    }
    while((bytes_read = recv(local_socket, buffer, BUFFER_SIZE, 0)) > 0){
        send(client_socket, buffer, bytes_read, 0); 
    }
    close(local_socket);

}


