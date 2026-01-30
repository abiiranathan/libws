#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/ws_server.h"

#define MAX_NAME_LEN 32

typedef struct {
    char name[MAX_NAME_LEN];
    char channel[MAX_NAME_LEN];
} user_ctx_t;

ws_server_t* chat_server = NULL;

typedef struct {
    const char* channel;
    ws_client_t* sender;
    bool exclude_sender;
} broadcast_args_t;

// Filter function for broadcast
bool channel_filter(ws_client_t* client, void* arg) {
    broadcast_args_t* args = (broadcast_args_t*)arg;
    user_ctx_t* ctx = (user_ctx_t*)ws_get_user_data(client);

    // Check if client has been initialized
    if (!ctx) return false;

    // Check if client is in the same channel
    if (strcmp(ctx->channel, args->channel) != 0) {
        return false;
    }

    // Check if we should exclude sender
    if (args->exclude_sender && client == args->sender) {
        return false;
    }

    return true;
}

void send_to_channel(ws_client_t* sender, const char* channel, const char* msg, bool exclude_sender) {
    char json[1024];
    user_ctx_t* sender_ctx = (user_ctx_t*)ws_get_user_data(sender);

    snprintf(json, sizeof(json), "{\"user\": \"%s\", \"channel\": \"%s\", \"message\": \"%s\"}", sender_ctx->name,
             channel, msg);

    broadcast_args_t args = {.channel = channel, .sender = sender, .exclude_sender = exclude_sender};

    ws_server_broadcast_text_filter(chat_server, json, channel_filter, &args);
}

void process_command(ws_client_t* client, char* text) {
    user_ctx_t* ctx = (user_ctx_t*)ws_get_user_data(client);

    text[strcspn(text, "\r\n")] = 0;

    if (text[0] == '/') {
        char* cmd = strtok(text, " ");
        char* arg = strtok(NULL, "");

        if (strcmp(cmd, "/nick") == 0 && arg) {
            strncpy(ctx->name, arg, MAX_NAME_LEN - 1);
            ws_send_text(client, "{\"type\": \"system\", \"message\": \"Nickname changed\"}");
        } else if (strcmp(cmd, "/join") == 0 && arg) {
            strncpy(ctx->channel, arg, MAX_NAME_LEN - 1);
            ws_send_text(client, "{\"type\": \"system\", \"message\": \"Joined channel\"}");
        } else {
            ws_send_text(client, "{\"type\": \"error\", \"message\": \"Unknown command\"}");
        }

    } else {
        send_to_channel(client, ctx->channel, text, false);
    }
}

// Callbacks
void on_open(ws_client_t* client) {
    user_ctx_t* ctx = malloc(sizeof(user_ctx_t));
    snprintf(ctx->name, MAX_NAME_LEN, "User%d", client->socket_fd);
    strcpy(ctx->channel, "general");
    ws_set_user_data(client, ctx);

    printf("Client connected (fd=%d)\n", client->socket_fd);

    ws_send_text(client, "{\"type\": \"welcome\", \"message\": \"Welcome! Commands: /nick <name>, /join <channel>\"}");
}

void on_message(ws_client_t* client, const uint8_t* data, size_t size, int type) {
    if (type == WS_OPCODE_TEXT) {
        char* text = malloc(size + 1);
        memcpy(text, data, size);
        text[size] = '\0';

        process_command(client, text);
        free(text);
    }
}

void on_close(ws_client_t* client, int code, const char* reason) {
    (void)code;
    (void)reason;
    printf("Client disconnected (fd=%d)\n", client->socket_fd);
    user_ctx_t* ctx = (user_ctx_t*)ws_get_user_data(client);
    if (ctx) free(ctx);
}

void handle_sigint(int sig) {
    (void)sig;
    printf("\nStopping server...\n");
    if (chat_server) ws_server_stop(chat_server);
}

int main(int argc, char** argv) {
    setbuf(stdout, NULL);
    signal(SIGINT, handle_sigint);

    ws_server_config_t config = {
        .port = 8081,
        .thread_count = 0,  // Auto-detect
        .on_open = on_open,
        .on_message = on_message,
        .on_close = on_close,
    };

    int arg_idx = 1;

    // Optional Port: ./chat_server [port]
    if (argc > arg_idx && isdigit(argv[arg_idx][0])) {
        config.port = atoi(argv[arg_idx]);
        arg_idx++;
    }

    // Optional SSL: ./chat_server [port] [cert] [key]  OR  ./chat_server [cert] [key]
    if (argc > arg_idx + 1) {
        config.ssl_cert = argv[arg_idx];
        config.ssl_key = argv[arg_idx + 1];
        printf("SSL/TLS Enabled. Cert: %s, Key: %s\n", config.ssl_cert, config.ssl_key);
    }

    chat_server = ws_server_create(&config);
    if (!chat_server) return 1;

    printf("Chat server running on port %d\n", config.port);
    ws_server_start(chat_server);
    ws_server_destroy(chat_server);

    return 0;
}
