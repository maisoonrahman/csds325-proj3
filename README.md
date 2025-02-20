# CSDS 325 Project 3
This is the third project for my Computer Networks I. This project is about implementing a simple HTTP
server in C++. The server should be able to listen for incoming TCP connections on a specified port, 
read an HTTP request sent by a client, parse the request to determine the requested file, serve the requested file
from a specified docuemnt directory or return an appropriate error response, and then close the TCP connection 
go back to listening for new requests.

Usage: ``` ./proj3 -p port -r document_directory -t auth_token ```

### Option ``` -p ```:
Specifies the port number the server should listen on.

### Option ``` -r ```: 
Specifies the root directory from which files should be served.

### Option  ``` -t ```:
A token for authentication. 
