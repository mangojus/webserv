_This project has been created as part
of the 42 curriculum by tfournie, abonifac, rshin_

# WebServ Project

## Description

This web server project is designed with robustness and strict testing in mind, especially focusing on **configuration file parsing**. Our goal is to ensure reliability and correctness by applying the **strictest testing rules** throughout the project.  

The server follows the **classic Nginx HTTP/1.0** behavior (ancient version, default) for request handling.

- The web server is **non-blocking**, enabling efficient handling of concurrent connections.  
- Each incoming request is handled by a **dedicated client instance**, ensuring isolation and proper resource management.  
- The server is optimized for stability and concurrency while maintaining a clear, testable architecture.  

## Instructions

- The server can be launched with a **single configuration file** specified by the user
    - If no configuration file is provided, the server will use a **default configuration**

### Configuration

- Configuration parsing is strictly validated to prevent invalid setups:
    - no duplicate directives
    - correct ranges

- Pathing needs to follow strict rules:
    - root path cannot be absolute nor relative, static files should stay in git repository for this project
    - cgi-pass needs to be absolute path
    - same for error-pages
    - location paths need to start with '/' (doesn't mean path is true absolute)

- Listen directive values are checked partially in config (wrong input/range, valid IPV4), the rest through bind in Listener class.

- Once config file has been parsed, it will be normalized:
    - Locations with unset directives will inherit from Server configs
    - Server with unset directives will set them to default configuration

- After normalization a final verification will be done to compare global configs:
    - no duplicate server-names between servers
    
- Location have strict cgi setups, only one cgi-extension and one cgi-pass allowed per Location and cgi directive must be specified in order to work.

Even though configuration follows strict parsing rules it's up to you to design your own secured infrastructure and know your config

### Booter

Booter is responsible for initializing the server from the configuration file. It parses the configuration, groups compatible server blocks together, and builds the list of Listener objects.

Each Listener represents one listening socket bound to a specific IP:port pair. A single listener can contain several server blocks, which is what allows virtual hosting on the same socket.

Booter also owns the lifetime of the Listener objects. It creates them during initialization and frees them when the program shuts down.

When no configuration file is provided, Booter falls back to a default configuration.

How listeners are built

During build_listeners(), the booter iterates over all parsed server configurations. For each config:

it reads the host and port,
replaces an empty host with 0.0.0.0,
builds a UniquePair(host, port),
checks whether a Listener already exists for that pair.

If no listener exists yet, a new Listener is created, stored in the map, added to the listener list, and the config is attached to it. If a listener already exists for the same pair, the config is simply added to the existing listener.

Once all server blocks have been grouped, the booter opens the socket of every listener.

UniquePair

UniquePair is used as the key in the listener map to guarantee that only one Listener exists for a given listening address.

This makes it possible to merge multiple server blocks into the same listener when they share the same socket. In practice, this is what supports virtual hosting: several server blocks can listen on the same IP:port, and the server later selects the correct one depending on the request.

In your project, this also enforces the rule that the same listening entry must stay unique, whether it is an explicit ip:port pair or a wildcard-style binding such as 0.0.0.0:port.

### Listener

A Listener stores:

the host, the port, the listening file descriptor,
the backlog, and the list of server configurations attached to this socket.

When several configs are attached to the same listener, the listener keeps the largest backlog value among them.

Socket creation

The socket is opened in initiate_socket().

This implementation only uses IPv4, which is why hints.ai_family is set to AF_INET. The socket type is SOCK_STREAM, meaning TCP is used.

getaddrinfo() is called to translate the host and port into one or more usable socket address structures. If the configured host is 0.0.0.0, the code uses AI_PASSIVE and passes NULL as the host, so the socket binds to all local IPv4 interfaces.

getaddrinfo() may return a linked list of possible addresses. The code iterates through this list and tries each candidate until one succeeds. For each candidate:

it creates a socket,
enables SO_REUSEADDR with setsockopt(),
then tries to bind() the socket.

As soon as one bind succeeds, the loop stops and that socket becomes the listening socket.

If no candidate works, an exception is thrown.

Why setsockopt(SO_REUSEADDR) is used

SO_REUSEADDR allows the address to be reused more easily after a restart. Without it, the server may fail to bind because the previous socket is still considered in use by the system for a short time.

What listen() does

After a successful bind, listen() turns the socket into a passive listening socket. In other words, the socket no longer represents one connection: it becomes a socket that waits for incoming client connections.

The backlog value passed to listen() defines how many pending connection requests can wait in the accept queue before the kernel starts refusing new ones.

Why non-blocking mode is used

The listening socket is then set to non-blocking mode with fcntl() and O_NONBLOCK.

Non-blocking mode is essential in an event-driven server. It prevents operations on the socket from freezing the whole process while waiting for network activity. This allows the engine to monitor many file descriptors at once and react only when they are ready.

### Engine

engine.run() is the main event loop of the server.

Its role is to continuously monitor the listening sockets, client sockets, and CGI-related file descriptors, then dispatch the appropriate action depending on which file descriptor becomes ready.

Inside this loop, wait_events() calls poll().

What poll() does

poll() monitors a set of file descriptors and tells the server which ones are ready for reading, writing, or have encountered an error.

In this project, poll() acts as the central guard of the server. Instead of blocking on a single socket operation, the server gives poll() the list of file descriptors it wants to watch, and poll() wakes up only when something meaningful happens.

For example, poll() can indicate that:

a listening socket is ready to accept a new client,
a client socket has incoming request data to read,
a client socket is ready to send response data,
a CGI pipe is ready to be written to or read from.
Why a timeout is set on poll()

A timeout is used on poll() to avoid blocking forever.

This is especially important for CGI handling. In some cases, the end of the CGI output stream and the end of the CGI process do not happen at exactly the same moment. Without a timeout, the server could remain stuck waiting indefinitely for an event that will not wake poll() on its own.

By waking up regularly, the engine can perform additional checks such as:

CGI timeout detection,
process state checks,
cleanup of desynchronized or stalled states.

This prevents infinite loading situations and makes the server more robust.

Engine and state machine

The _fdCtx map allows us to know what kind of fd we are handling, like listeners, clients or CGI pipes.

After that, the engine is built as a state machine.

Each step is clearly defined (READING_HEADER, READING_BODY, REQUEST_COMPLETE, SENDING_RESPONSE, etc.). Each client has its own state, so when the main loop runs again, the client can continue directly at the correct step.

For a simple valid GET request, the flow is usually:

READING_HEADER -> REQUEST_COMPLETE -> BUILDING_STATIC_RESPONSE -> SENDING_RESPONSE

After that, poll() checks when the client fd is ready for writing with POLLOUT, so the server can send the response.

CGIs are handled as a branch of the client. Their pipe fds are also watched by poll().

When the CGI is fully finished, its output is copied into the client output buffer. Then the client buffer is responsible for sending the final response to the socket.

### Request

Request.hpp/.cpp
Plain data container: method, path, query, version, headers (string→string map), body, content_len, chunked. No parsing logic — only getters and setters. This is the object that travels between the parser, the client, and the response builder.

### HttpParser

HttpParser.hpp/.cpp
Takes the raw header (up to \r\n\r\n) and fills a Request object. Enforces singleton headers via isUniqueHeader() (host, content-length, transfer-encoding…) mirroring nginx behaviour — duplicates are rejected for those keys. Also extracts chunked and content-length flags to drive the rest of the body reading in Client.

### Client — state machine

Client.hpp/.cppClient_state.cpp
Core of the network flow. Each client progresses through a fixed sequence: READING_HEADER → READING_BODY / READING_CHUNK → REQUEST_COMPLETE → CGI_* / BUILDING_STATIC_RESPONSE → SENDING_RESPONSE. Incoming bytes accumulate in _in_buffer, the outgoing response in _out_buffer. Chunked bodies are decoded in-place into _decoded_body.

### Client — server & location selection

Client_request.cpp
Once the request is complete, select_server() picks the virtual host by matching the Host header against server names on the same port (fallback: "default", then first config on that port). select_location() finds the longest matching location prefix. compare_ConfReq() then checks that the method is allowed and that the body does not exceed client_max_body_size before proceeding.

### Client — path resolution

Client_path.cpp
resolve_path() concatenates root + URI after sanitising for length, forbidden characters, path traversal, and null bytes. resolve_final_path() stat()s the result: directory → resolve_directory() (tries index file, falls back to autoindex or 403/405), regular file → check_permissions() then returns the final path. _isdir is set here so the CGI/static dispatch downstream is correct.

### Response

Response.hpp/.cpp
Builds the final HTTP response. serveGet() reads the file in binary mode or generates a directory listing via buildAutoindex(). servePost() writes the request body to the target file (201 on creation, 200 on update). serveDelete() removes the file. buildErrorResponse() looks up a custom error_page in the config, falling back to plain text. buildValidResponse() assembles the status line, Content-Type (detected by extension), Content-Length, and body.

flow: network → Client (states) → HttpParser → Request → path resolution → Response → network


# webserv — curl test sequence

## GET — happy path

### 01 · serve index page `200`
Basic GET on the default vhost — checks root + index resolution.
```bash
curl -v -H "Host: default" http://localhost:8080/
```
> Expected: `200 OK` · index.html body

---

### 02 · redirect `301`
Hits `/redirect` — verifies the `return 301` directive.
```bash
curl -v -H "Host: default" http://localhost:8080/redirect
```
> Expected: `301` · `Location: http://google.com`

---

### 03 · autoindex `200`
GET on `/upload` (autoindex on, no index file) — shows directory listing.
```bash
curl -v -H "Host: default" http://localhost:8080/assets/
```
> Expected: `200 OK` · HTML directory listing

---

## POST + DELETE — upload lifecycle

### 04 · upload a file `201`
POST a text file to `/upload` — verifies write + 201 Created.
```bash
curl -v -X POST -H "Host: default" \
  --data-binary "hello webserv" \
  http://localhost:8080/upload/
```
> Expected: `201 Created`

---

### 05 · retrieve the uploaded file `200`
GET the file just created — confirms it was written correctly.
```bash
curl -v -H "Host: default" http://localhost:8080/upload/[upload_name...]
```
> Expected: `200 OK` · body: `hello webserv`

---

### 06 · delete the file `200`
DELETE the same file — verifies removal + 200.
```bash
curl -v -X DELETE -H "Host: default" http://localhost:8080/upload/[upload_name...]
```
> Expected: `200 OK` · `File deleted`

---

## Error handling — rejections

### 07 · method not allowed `405`
DELETE on `/contact` (only GET POST allowed) — verifies method check.
```bash
curl -v -X DELETE -H "Host: default" http://localhost:8080/contact/
```
> Expected: `405 Method Not Allowed` · custom error page

---

### 08 · body too large `413`
POST 25 KB to default vhost (`client_max_body_size 24000`) — verifies size enforcement.
```bash
curl -v -X POST -H "Host: default" \
  --data-binary "$(python3 -c 'print("A"*25000)')" \
  http://localhost:8080/upload/big.txt
```
> Expected: `413 Payload Too Large` · custom error page

---

### 09 · file not found `404`
GET on a path that does not exist — verifies 404 + custom error page.
```bash
curl -v -H "Host: default" http://localhost:8080/doesnotexist
```
> Expected: `404 Not Found` · custom error page

---

## Virtual host

### 10 · vhost isolation `200`
Same port, different Host header — confirms `gallery.localhost` serves its own root.
```bash
curl -v -H "Host: gallery.localhost" http://localhost:8080/
```
> Expected: `200 OK` · gallery.com index, not webserv.com

## Chunk

### 11 · chunk request `201`
```bash
printf "POST /contact/ HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: chunked\r\nContent-Length: 10\r\n\r\n5\r\nhello\r\n0\r\n\r\n" | nc localhost 8080
```
> Expected: `200 OK`


### 12 · vhost test `200`

```bash
curl -H "Host: websev.localhost" http://localhost:8080/
```
> Expected: `200 OK`


## Resources

    - https://www.rfc-editor.org/rfc/rfc9112
    - https://nginx.org/en/
    - https://datatracker.ietf.org/doc/html/rfc2616
    - https://www.webfx.com/web-development/glossary/http-status-codes/
    - https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference
    - https://www.codequoi.com/programmation-reseau-via-socket-en-c/

    - run schematics in resources folder

AI was used accordingly with 42 AI rules section.
    - to reduce tedious tasks i.e : readme paragraph correction
    - help with better refactoring/const correctness checks, easy to miss global and small checks
    - testers, and ideas of tests