# Friendlist Application

A robust web-based friend-graph manager, the Friendlist Application is engineered to manage dynamic friend relationships via HTTP. Developed in C with a custom server framework, this application provides an intuitive web interface for effective user connection management.

## Installation

1. **Clone the Repository:** Access the latest version from our Git repository.
2. **Compile the Server:** Navigate to the project directory and run the `make` command to build the application.
3. **Start the Server:** Launch the server using `./friendlist <port>`, replacing `<port>` with a suitable number between 1024 and 65535, depending on your network setup.

## Key Features

- **Dynamic User Management:** Automates user registration and friendship management based on incoming HTTP requests.
- **Concurrent Connections:** Employs multithreading to manage multiple client connections simultaneously, ensuring high availability and robust error handling.
- **Friendship Operations:** Enables complex operations such as adding friends, removing friends, and facilitating friend introductions across servers.

## Initializing the Server

```
make
./friendlist <port>
```
Replace <port> with a number between 1024 and 65535. Note that only ports 2100 - 2120 are externally accessible on CADE machines. You can connect to the server using:
```
http://localhost:<port>/
```
Upon connection, the server initially lists two friends: Alice and Bob. It also outputs any received query parameters.

## Server Operations
Get Friends: Retrieves a user's friends list.
```
curl "http://localhost:8090/friends?user=alice"
```
Befriend: Adds users to another user's friends list.
```
curl "http://localhost:8090/befriend?user=me&friends=alice"
```
Unfriend: Removes users from a user's friends list.
```
curl "http://localhost:8090/unfriend?user=me&friends=alice"
```
Introduce: Connects users across different servers to expand friend networks.
```
curl "http://localhost:8090/introduce?user=me&friend=alice&host=localhost&port=8090"
```
## Technical Highlights
- Thread-Safe Operations: Uses mutexes to ensure all client interactions are secure and reliable.
- Efficient Data Storage: Leverages custom dictionaries for streamlined user data and relationship management.
- Error Handling: Delivers robust error responses to invalid requests to maintain clear and continuous server operation.
- Security and Performance
- Client Isolation: Designed to isolate and manage misbehaving clients without impacting server performance.
- Memory Management: Implements strategies for efficient memory use and leak prevention to support extensive data handling and client requests.
## Example Output
```
$ curl "http://localhost:8090/befriend?user=me&friends=alice"
alice
$ curl "http://localhost:8090/befriend?user=me&friends=alice"
alice
$ curl "http://localhost:8090/befriend?user=me&friends=bob"
alice
bob
$ curl "http://localhost:8090/friends?user=alice"
me
$ curl "http://localhost:8090/befriend?user=alice&friends=bob%0Acarol"
me
bob
carol
$ curl "http://localhost:8090/unfriend?user=me&friends=alice"
bob
$ curl "http://localhost:8090/introduce?user=me&friend=alice&host=localhost&port=8090"
$ curl "http://localhost:8090/friends?user=me"
bob
carol
alice
$ curl "http://localhost:8090/friends?user=alice"
bob
carol
me
```
### Test output on stress.rkt
```
$ make
cc -O2 -g -Wall -I. -o friendlist friendlist.c dictionary.c more_string.c csapp.c -pthread
$ ( ulimit -v 20971520 ; ./friendlist 8090 > /dev/null )
```
```
$ racket stress.rkt localhost 8090
friending
friending
friending
friending
checking friends
checking friends
checking friends
unfriending
checking friends
unfriending
unfriending
unfriending
checking friends
checking friends
checking friends
checking friends
introducing
introducing
introducing
introducing
checking friends
checking friends
checking friends
checking friends
```
