# Friendlist Application
A robust web-based friend-graph manager designed to handle dynamic friend relationships over HTTP. Developed using C and a custom server framework, this application offers a practical interface for managing user connections through a web browser.

## Installation
Clone the repository: Obtain the latest version from our Git repository.
Compile the server: Navigate to the project directory and run make to build the application.
Start the server: Execute ./friendlist <port> where <port> is a number between 1024 and 65535, suitable for your network configuration.

## Key Features
- Dynamic User Management: Automatically registers users and manages friendships as dictated by HTTP requests.
- Concurrent Connections: Utilizes multithreading to handle multiple clients simultaneously, ensuring high availability and robust error handling.
- Friendship Operations: Supports complex operations such as adding friends, removing friends, and introducing friends from different servers.
## Server Operations
Get Friends: Retrieve a list of friends for any registered user.
`curl "http://localhost:8090/friends?user=alice"`
Befriend: Add one or more users to another user's friend list, automatically reciprocating the friendship.
`curl "http://localhost:8090/befriend?user=me&friends=alice"`
Unfriend: Remove one or more users from a user's friend list.
`curl "http://localhost:8090/unfriend?user=me&friends=alice"`
Introduce: Facilitate the introduction of users from one server to the users of another server, extending friend networks.
`curl "http://localhost:8090/introduce?user=me&friend=alice&host=localhost&port=8090"`
## Technical Highlights
Thread-Safe Operations: Ensures that all client interactions are thread-safe using mutexes.
Efficient Data Storage: Uses custom dictionaries for storing user data and relationships.
Error Handling: Robust error responses for invalid requests, ensuring clarity and continuous server operation.
## Security and Performance
Client Isolation: Designed to prevent a single misbehaving client from affecting the server's overall performance.
Memory Management: Implements efficient memory usage and leak prevention to handle large volumes of data and requests.
