_This project has been created as part of the 42 curriculum by tfournie, abonifac, rshin._

# webserv

An HTTP/1.1 web server written from scratch in **C++98**, modeled on nginx: non-blocking I/O with a single `poll()` loop, virtual hosts, an nginx-style configuration file, static files, uploads, and CGI.

## Features

- **Single `poll()` event loop**: listeners, client sockets and CGI pipes are all non-blocking and multiplexed together. No thread per connection, no blocking reads.
- **Per-client state machine**: `READING_HEADER → READING_BODY / READING_CHUNK → REQUEST_COMPLETE → CGI / STATIC → SENDING_RESPONSE`, so a slow client never blocks the others.
- **Methods**: `GET`, `POST`, `DELETE`.
- **Virtual hosting**: several `server` blocks on the same `ip:port`, selected by the `Host` header (fallback: `default`, then the first block on that port).
- **Locations**: longest-prefix match with per-location `root`, `index`, `autoindex`, `allowed_methods`, `return` (redirects) and CGI settings.
- **CGI**: Python, PHP and Perl through `fork`/`execve`, with request data passed as environment variables plus the body on stdin. Timeouts are enforced.
- **Request bodies**: `Content-Length` and `Transfer-Encoding: chunked`, with `client_max_body_size` enforced (`413`).
- **Errors**: custom `error_page` per status code, falling back to built-in pages.
- **Hardening**: path-traversal and forbidden-character checks, duplicate singleton headers rejected (`Host`, `Content-Length`, …), URI length limit (`414`), client timeouts.
- **Strict config validation**: duplicate directives, out-of-range values, bad paths and duplicate `server_name`s are all rejected at startup, with the offending line number.

## Build & run

Requirements: a C++98 compiler and `make`. For the CGI demos: `python3`, plus optionally `php` and Perl with `CGI.pm` (`libcgi-pm-perl`).

```bash
make
./webserv config.conf      # or ./webserv to use the built-in default config
```

The sample [`config.conf`](config.conf) serves three sites on port **8080**:

| URL | Site | Root |
|---|---|---|
| http://localhost:8080 | demo site (server `default`) | `www/webserv.com/html` |
| http://gallery.localhost:8080 | image gallery (upload/list/delete via CGI) | `www/gallery.com/html` |
| http://webserv.localhost:8080 | minimal static page | `www/html` |

`*.localhost` resolves to your machine in browsers and curl, so no `/etc/hosts` change is needed.

> **Uploads:** for safety, `POST` and `DELETE` only work when the target directory is world-writable. Create it first:
> ```bash
> mkdir -p www/webserv.com/html/upload www/gallery.com/html/gallery
> chmod 777 www/webserv.com/html/upload www/gallery.com/html/gallery
> ```

## Configuration

```nginx
server {
    listen 8080 backlog=50;
    server_name default;
    root www/webserv.com/html;
    index index.html;
    allowed_methods GET DELETE;
    client_max_body_size 24000;

    location / {}

    location /redirect { return 301 http://google.com; }

    location /upload   { allowed_methods GET POST DELETE; }

    location /cgi-bin {
        root www/webserv.com;
        allowed_methods GET POST;
        cgi;
        cgi_extension .py;
        cgi_pass /usr/bin/python3;
    }

    error_page 404 errors/404.html;
}
```

The rules are strict on purpose:

- A directive can't be repeated within a block (except `error_page` with different status codes), and values are range-checked.
- `root` and `error_page` paths are relative to the repository, without a leading `/`, `./` or `..`. `cgi_pass` must be an absolute path. Location paths start with `/`.
- Each location allows one `cgi_extension` and one `cgi_pass`, and the `cgi` directive must be present for CGI to run.
- After parsing, the config is normalized: locations inherit unset directives from their server, and servers fall back to defaults.
- `server_name` must be unique across servers.

## Try it

With the server running (`./webserv config.conf`):

```bash
curl -i http://localhost:8080/                       # 200 static index
curl -i http://localhost:8080/assets/                # 200 autoindex listing
curl -i http://localhost:8080/redirect               # 301 → google.com
curl -i http://localhost:8080/cgi-bin/date.py        # 200 Python CGI
curl -i http://localhost:8080/cgi-bin/env.py         # 200 CGI environment dump
curl -i http://localhost:8080/doesnotexist           # 404 custom error page
curl -i -X DELETE http://localhost:8080/contact/     # 405 method not allowed

# upload lifecycle (create the upload dir first, see above)
curl -i -X POST --data-binary "hello webserv" http://localhost:8080/upload/   # 201 → upload_<timestamp>
curl -i http://localhost:8080/upload/                                          # find the file name
curl -i -X DELETE http://localhost:8080/upload/upload_<timestamp>              # 200

# body too large (limit is 24000 bytes)
curl -i -X POST --data-binary "$(head -c 25000 /dev/zero | tr '\0' A)" http://localhost:8080/upload/big.txt   # 413

# chunked request
printf 'POST /upload/chunk.txt HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello\r\n0\r\n\r\n' | nc localhost 8080   # 201

# virtual host on the same port
curl -i http://gallery.localhost:8080/               # 200 gallery index
```

## Project structure

```
src/
├── config/   ConfigLexer → ConfigParser → ConfigValidator (tokenize, parse, normalize, validate)
├── engine/   Booter (builds listeners), Listener (socket/bind/listen), Engine (poll loop, CGI, timeouts, signals)
├── client/   per-connection state machine, server/location selection, path resolution, CGI bridge
├── http/     HttpParser, Request, Response, Cgi, CgiResponse
├── utils/    RAII guards, string helpers, signals
└── debug/    debug printers
www/          demo sites and CGI scripts
```

For a walkthrough of each component (Booter, Listener, Engine, Client, Response) and design decisions such as why `poll()` has a timeout, see [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) and the run schematic in [`resources/runSchematic.png`](resources/runSchematic.png).

## Team

- **abonifac**: sockets, event loop and engine, client handling
- **tfournie**: HTTP parsing and requests, client handling, security tests
- **rshin**: configuration lexer/parser/validator, engine, HTTP responses

## Resources

- [RFC 9112, HTTP/1.1](https://www.rfc-editor.org/rfc/rfc9112) · [RFC 2616](https://datatracker.ietf.org/doc/html/rfc2616)
- [nginx documentation](https://nginx.org/en/docs/)
- [MDN HTTP reference](https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference)
- [Network programming with sockets in C (codequoi)](https://www.codequoi.com/programmation-reseau-via-socket-en-c/)

AI was used in line with 42's AI rules: proofreading documentation, and help with refactoring and const-correctness checks.
