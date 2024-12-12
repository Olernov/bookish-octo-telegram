# Simple one-on-one chat application

This is a simple console chat application allowing two users to send each other
messages over TCP/IP network.
It is implemented as an asynchronous server/client and uses boost:asio library.

## Usage
  `chat mode [host] [port]`

    mode    `srv|cli`    `srv` starts the program in server mode (accepting incoming connections);
                       `cli` - in client mode (program connects to the host)  
    host                 IP-address or host name to connect to (only for client mode)  
    port                 TCP/IP port used for connection. Default 5401  

Once started in the `server` mode, the program waits for incoming connections on the given port.
If the program is started in the `client` mode, it first tries to establish connection
to the server on the given host address and port.

After the connection is established, participants can start exchanging messages.
Just print the message in the command line and press `[Enter]`.
Incoming messages from the remote side are prompted by `>>>`.
After each successfully delivered message the program prints time elapsed between
sending and delivery notification receipt.

Server accepts more than one connection but sends messages to the most
recently connected session only.

Press `[Ctrl-C]` to stop the program.

## Build instructions
### Pre-requisites
  `sudo apt install g++ libboost-all-dev`  
  or  
  `sudo yum install g++ libboost-all-dev`

### Build
    mkdir build
    cd build
    cmake ..
    cmake --build .
