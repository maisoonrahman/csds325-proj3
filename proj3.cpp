/* ==================================================================================================
==============================  author: Maisoon A Rahman ============================================
==============================       caseID: mar296      ============================================
==============================      Assignment: proj1    ============================================ 
========================== description: this is the answer to project 3 =============================
*/

#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>   
#include <cstring>   
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define ERROR 1
#define QLEN 1
#define BUFLEN 1024

// declare all functions used 
void setup_server(const std::string &port, const std::string &doc_dir, const std::string &auth_token);
void handle_request(int client_sd, const std::string &doc_dir, const std::string &auth_token);
void send_response(int client_sd, const std::string &response);


void usage(const char *progname) {
    std::cerr << "Usage: " << progname << " -p port -r document_directory -t auth_token\n";
    exit(ERROR);
}

void errexit(const char *format, const char *arg = nullptr) {
    std::cerr << format;
    if (arg != nullptr) {
        std::cerr << ": " << arg;
    }
    std::cerr << std::endl;
    exit(ERROR);
}

int main(int argc, char *argv[]) {
    std::string port;
    std::string doc_dir;
    std::string auth_token;

for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "-p") == 0) {
            if (i + 1 < argc) {
                port = argv[++i]; 
            } else {
                usage(argv[0]);
            }
        } else if (std::strcmp(argv[i], "-r") == 0) {
            if (i + 1 < argc) {
                doc_dir = argv[++i]; 
            } else {
                usage(argv[0]);
            }
        } else if (std::strcmp(argv[i], "-t") == 0) {
            if (i + 1 < argc) {
                auth_token = argv[++i]; 
            } else {
                usage(argv[0]);
            }
        } else {
            usage(argv[0]); 
        }
    }


    if (port.empty() || doc_dir.empty() || auth_token.empty()) {
        usage(argv[0]);
    }

    setup_server(port, doc_dir, auth_token);

    return 0;

}

void setup_server(const std::string &port, const std::string &doc_dir, const std::string &auth_token) {
    struct sockaddr_in sin;
    int sd, new_sd;
    socklen_t addrlen = sizeof(sin);

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(std::stoi(port));

    sd = socket(PF_INET, SOCK_STREAM, 0);
    if (sd < 0) {
        errexit("cannot create socket");
    }

    if (bind(sd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        errexit("cannot bind to port", port.c_str());
    }

    if (listen(sd, QLEN) < 0) {
        errexit("cannot listen on port", port.c_str());
    }

    // for debugging purposes: std::cout << "server listening on port" << port << std::endl;

    while(true) {
        new_sd = accept(sd, (struct sockaddr *)&sin, &addrlen);
        if (new_sd < 0) {
            errexit("error accepting connection");
        }

        handle_request(new_sd, doc_dir, auth_token);
        close(new_sd);
    }

    close(sd);
}

void handle_request(int client_sd, const std::string &doc_dir, const std::string &auth_token) {
    char buffer[BUFLEN];
    int ret;

    memset(buffer, 0, BUFLEN);
    ret = read(client_sd, buffer, BUFLEN - 1);
    if (ret < 0) {
        errexit("error reading from socket");
    }

    // for debugging purposes: std::cout << "received request:\n" << buffer << std::endl;

    // ensuring the request ends with "\r\n\r\n" (a blank line)
    // this is also the first check in the priority list 
    std::string request_str(buffer);
    if (request_str.find("\r\n\r\n") == std::string::npos) {
        send_response(client_sd, "HTTP/1.1 400 Malformed Request\r\n\r\n");
        close(client_sd);
        return;
    }

    // check end of each line for "\r\n"
    std::istringstream request_stream(request_str);
    std::string line;
    while (std::getline(request_stream, line)) {
        if (line.empty()) break; // Stop after the blank line at the end of headers

        // Check if the line ends with \r (getline() removes \n, so we check only \r)
        if (line.back() != '\r') {
            send_response(client_sd, "HTTP/1.1 400 Malformed Request\r\n\r\n");
            close(client_sd);
            return;
        }
    }

    std::istringstream first_line_stream(request_str);
    std::string method, argument, http_version;
    std::istringstream request(buffer);
    first_line_stream >> method >> argument >> http_version;

     // checking for any missing components in the first line
    if (method.empty() || argument.empty() || http_version.empty()) {
        send_response(client_sd, "HTTP/1.1 400 Malformed Request\r\n\r\n");
        close(client_sd);
        return;
    }


    // checking the HTTP version is valid 
    // second response in the priority list
    if (http_version.rfind("HTTP/", 0) != 0) {
        send_response(client_sd, "HTTP/1.1 501 Protocol Not Implemented\r\n\r\n");
        return;
    }

    // if neither GET or SHUTDOWN is the method
    // third response in the priority list
    if (method != "GET" && method != "SHUTDOWN") {
        send_response(client_sd, "HTTP/1.1 405 Unsupported Method\r\n\r\n");
        close(client_sd);
        return;
    }

    // if method is SHUTDOWN
    // fourth response in the priority list
    if (method == "SHUTDOWN") {
       if (argument == auth_token) {
        send_response(client_sd, "HTTP/1.1 200 Server Shutting Down\r\n\r\n"); // first in the shutdown sublist response priority list
        // for debugging purposes: std::cout << "Server is shutting down" << std::endl;
        close(client_sd);
        exit(0);
       }
       else {
        send_response(client_sd, "HTTP/1.1 403 Operation Forbidden\r\n\r\n"); // second in the shutdown sublist response priority list
        close(client_sd);
        return;  // server is still running here
       }
    }

    // if method is GET
    // fifth response in the priority list
    if (method == "GET") {
        // if requested file does not start with "/", return 
        if (argument[0] != '/') {
            send_response(client_sd, "HTTP/1.1 406 Invalid Filename\r\n\r\n"); // first in the get sublist response priority list
            close(client_sd);
            return;
        }

        // serve "/index.html" by default for only "/"
        if (argument == "/") {
            argument = "/index.html";  
        }

        std::string full_path = doc_dir + argument;

        std::ifstream file(full_path);

        
        if (!file) {
            // file does not exist or cannot be opened
            send_response(client_sd, "HTTP/1.1 404 File Not Found\r\n\r\n"); // third in the get sublist response priority list
            close(client_sd);
            return;
        }
        
        // if the file exists, send a 200 OK response
        send_response(client_sd, "HTTP/1.1 200 OK\r\n\r\n"); // second in the get sublist response priority list

        // read the file and send its contents in chunks to avoid memory issues with larger files
        char file_buffer[BUFLEN];
        while (!file.eof()) {
            file.read(file_buffer, BUFLEN);
            int bytes_read = file.gcount();
            write(client_sd, file_buffer, bytes_read);
        }
        file.close();
        close(client_sd);
        return;

    }

}

void send_response(int client_sd, const std::string &response) {
    write(client_sd, response.c_str(), response.size());
    close(client_sd);
}
