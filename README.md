# WebSocket C Library

A robust, thread-safe, and feature-rich WebSocket (RFC 6455) implementation in C, featuring secure TLS/SSL support (wss://).

## Features

- **TLS/SSL Support**: Secure connections (wss://) using OpenSSL with certificate verification and SNI.
- **Thread Safety**: Full thread safety with mutex protection for all public APIs.
- **RFC 6455 Compliant**: Passes Autobahn test suite requirements (fragmentation, masking, UTF-8, control frames).
- **Comprehensive Error Handling**: Structured error codes (`WS_ERR_*`) and human-readable strings via `ws_strerror()`.
- **Fragmentation Support**: Automatic message fragmentation and reassembly.
- **UTF-8 Validation**: Strict RFC 3629 UTF-8 validation for text frames and close reasons.
- **Security**: 
  - Uses `/dev/urandom` for masking keys.
  - Configurable payload limits to prevent DoS.
  - Proper state machine enforcement.
- **Monitoring**: Built-in statistics (frames sent/received, bytes, ping/pong tracking).
- **Flexible IO**: Custom read/write callbacks supported (easy integration with SSL/TLS or event loops).

## Project Structure

```
.
├── Makefile            # Build system
├── include
│   ├── websocket.h     # Public API header (Client/Core)
│   ├── ws_client_lib.h # Helper for blocking clients
│   └── ws_server.h     # Server API header
├── src
│   ├── websocket.c     # Core library implementation
│   ├── ws_client_lib.c # Client helper implementation
│   └── ws_server.c     # Multi-threaded Epoll Server implementation
├── examples
│   ├── chat_client.c   # Interactive CLI Chat Client
│   └── chat_server.c   # Interactive Chat Server
└── build               # Build artifacts (binaries, libs)
```

## Building

To build the library and examples:

```bash
make
```

**Dependencies:**
- OpenSSL (`libssl-dev`)
- Pthread (`-pthread`)

## Running the Examples

### Chat Server

The server supports both standard (ws://) and secure (wss://) connections, and configurable ports.

**Usage:**
```bash
# Standard HTTP (ws://) on default port 8081
./build/bin/chat_server

# Standard HTTP (ws://) on custom port
./build/bin/chat_server 9000

# Secure HTTPS (wss://) on default port 8081
./build/bin/chat_server cert.pem key.pem

# Secure HTTPS (wss://) on custom port
./build/bin/chat_server 8443 cert.pem key.pem
```

To generate a self-signed certificate for testing:
```bash
openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365 -nodes -subj "/CN=localhost"
```

### Chat Client

The client connects to WebSocket servers, automatically detecting `ws://` vs `wss://`.

**Usage:**
```bash
# Connect to standard server (defaults to ws://localhost:8081)
./build/bin/chat_client

# Connect to specific URL
./build/bin/chat_client ws://localhost:9000

# Connect securely (TLS)
./build/bin/chat_client wss://localhost:8443
```

## API Overview

### Initialization & Configuration

```c
#include "websocket.h"

ws_client_t client;
ws_init(&client);

// Set callbacks
client.on_open = on_open;
client.on_message = on_message;
client.on_close = on_close;
client.on_error = on_error;

// Configuration
ws_set_max_payload_size(&client, 1024 * 1024); // 1MB limit
ws_set_auto_fragment(&client, true, 4096);     // Fragment outgoing > 4KB
ws_set_validate_utf8(&client, true);           // Enforce UTF-8 checking
```

### Connection (Client Mode)

The library automatically handles TLS if port 443 is used, or if explicitly enabled.

```c
// Manual TLS configuration (if not using standard 443)
ws_set_ssl(&client, true);

ws_error_t err = ws_connect(&client, "localhost", 8443, "/");
if (err != WS_OK) {
    fprintf(stderr, "Connect failed: %s\n", ws_strerror(err));
}
```

### Reading Data

The library provides `ws_read` which abstracts away the underlying SSL vs Socket read logic.

```c
// In your read loop
uint8_t buffer[4096];
ssize_t n = ws_read(&client, buffer, sizeof(buffer));

if (n > 0) {
    ws_consume(&client, buffer, (size_t)n);
}
```

### Server Side (Handshake)

```c
// After accepting a TCP connection
ws_accept(&client, client_fd);
```

### Sending Messages

```c
// Send Text
ws_send_text(&client, "Hello World", 11);

// Send Binary
uint8_t data[] = {0x01, 0x02, 0x03};
ws_send_binary(&client, data, sizeof(data));

// Send Ping (Pong is handled automatically)
ws_send_ping(&client, NULL, 0);
```

### Thread Safety

The library uses `pthread_mutex` internally. You can safely call `ws_send_*` or `ws_close` from multiple threads.

### Statistics

```c
ws_statistics_t stats;
ws_get_statistics(&client, &stats);
printf("Bytes Received: %lu\n", stats.bytes_received);
```

## Error Codes

The library uses `ws_error_t` for return values:

- `WS_OK`
- `WS_ERR_ALLOCATION_FAILURE`
- `WS_ERR_INVALID_PARAMETER`
- `WS_ERR_CONNECT_FAILED`
- `WS_ERR_SSL_FAILED`
- `WS_ERR_CERT_VALIDATION_FAILED`
- `WS_ERR_PROTOCOL_VIOLATION`
- `WS_ERR_PAYLOAD_TOO_LARGE`
- ... and more.

Use `ws_strerror(err)` to get the string representation.

## License

MIT
