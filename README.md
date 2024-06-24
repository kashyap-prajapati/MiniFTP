# MiniFTP

# Client-Server File Request Project

This project is a client-server application where a client can request a file or a set of files from the server. The server searches for the requested file(s) in its directory tree rooted at its home directory and returns the file(s) or an appropriate message to the client. Multiple clients can connect to the server from different machines and request files as per the defined commands.

## Project Structure

### 1. Server (`serverw24`)

- **Description**: The main server that handles file requests from clients.
- **Functionality**:
  - Waits for client connections.
  - Forks a child process for each client connection to handle requests in the `crequest()` function.
  - Handles commands like directory listing, file details, file search, and quit.
  - Alternates between handling connections with two mirror servers (`mirror1` and `mirror2`).

### 2. Mirror Servers (`mirror1` and `mirror2`)

- **Description**: Identical copies of `serverw24` with possible additional changes.
- **Functionality**: Same as `serverw24`, handling requests from clients in an alternating manner.

### 3. Client (`clientw24`)

- **Description**: The client process that sends commands to the server.
- **Functionality**:
  - Runs in an infinite loop, waiting for user commands.
  - Verifies the syntax of commands and sends them to the server.
  - Prints the response from the server.

## Commands

### Client Commands

- **`dirlist -a`**: Requests a list of subdirectories under the server's home directory in alphabetical order.
  ```sh
  clientw24$ dirlist -a
  ```

- **`dirlist -t`**: Requests a list of subdirectories under the server's home directory in the order of creation.
  ```sh
  clientw24$ dirlist -t
  ```

- **`w24fn <filename>`**: Requests details of a specific file if it exists in the server's directory tree.
  ```sh
  clientw24$ w24fn sample.txt
  ```

- **`w24fz <size1> <size2>`**: Requests a tarball of files whose sizes are between `size1` and `size2`.
  ```sh
  clientw24$ w24fz 1240 12450
  ```

- **`w24ft <extension list>`**: Requests a tarball of files with specified extensions (up to 3 types).
  ```sh
  clientw24$ w24ft c txt
  ```

- **`w24fdb <date>`**: Requests a tarball of files created before or on the specified date.
  ```sh
  clientw24$ w24fdb 2023-01-01
  ```

- **`w24fda <date>`**: Requests a tarball of files created on or after the specified date.
  ```sh
  clientw24$ w24fda 2023-03-31
  ```

- **`quitc`**: Sends a quit command to the server and terminates the client process.
  ```sh
  clientw24$ quitc
  ```

## Alternating Server Connections

- **Server Rotation**: 
  - First 3 client connections are handled by `serverw24`.
  - Next 3 connections (4-6) are handled by `mirror1`.
  - Next 3 connections (7-9) are handled by `mirror2`.
  - Remaining connections alternate between `serverw24`, `mirror1`, and `mirror2`.

## Usage

### Server Setup

1. **Start the Main Server**:
    ```sh
    ./serverw24
    ```

2. **Start Mirror Servers**:
    ```sh
    ./mirror1
    ./mirror2
    ```

### Client Setup

1. **Start the Client**:
    ```sh
    ./clientw24
    ```

### Directory Structure

- **Server Home Directory**: `~/`
- **Client Working Directory**: `~/w24project`

## Error Handling

- The client verifies command syntax before sending requests to the server.
- Appropriate error messages are printed for invalid commands.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for more details.

