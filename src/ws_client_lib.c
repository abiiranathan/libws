#include "../include/ws_client_lib.h"
#include <errno.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <stdio.h>
#include <unistd.h>

void ws_client_run(ws_client_t* client) {
    uint8_t buffer[4096];

    while (client->state != WS_STATE_CLOSED) {
        ssize_t n = ws_read(client, buffer, sizeof(buffer));

        if (n < 0) {
            if (client->use_ssl && client->ssl) {
                int err = SSL_get_error(client->ssl, (int)n);
                if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) continue;
                if (client->on_error) client->on_error(client, "SSL Read error");
            } else {
                if (errno == EINTR) continue;
                if (client->on_error) client->on_error(client, "Read error");
            }
            break;
        } else if (n == 0) {
            // EOF
            if (client->on_close) client->on_close(client, WS_STATUS_ABNORMAL, "Connection closed by peer");
            break;
        }

        ws_consume(client, buffer, (size_t)n);
    }
}
