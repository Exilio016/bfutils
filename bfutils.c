#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <openssl/ssl.h>
#include <sys/socket.h> 
#include <sys/types.h> 
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <poll.h>

#define BFUTILS_VECTOR_IMPLEMENTATION
#include "bfutils_vector.h"

void request_file(SSL *ssl, const char *name) {
    char *req = string_format("GET /Exilio016/bfutils/refs/heads/main/%s HTTP/1.1\r\nHost: raw.githubusercontent.com\r\nAccept: *\r\n\r\n", name);
    SSL_write(ssl, req, vector_length(req));
    vector_free(req);
}

char *get_file_response(SSL *ssl) {
    char *msg = NULL;
    char *body = NULL;
    do {
        char buffer[1024] = {0};
        int l = SSL_read(ssl, buffer, 1024);
        if (l < 0 && SSL_get_error(ssl, l) != SSL_ERROR_WANT_READ) {
            fprintf(stderr, "Connection error\n");
            exit(1);
        }
        string_push_cstr(msg, buffer);
        if (SSL_pending(ssl) <= 0) {
            break;
        }
    } while(1);
    if (0 != strncmp("HTTP/1.1 200 OK", msg, 14)) {
        vector_free(msg);
        return NULL;
    }
    long length = 0;
    for (int i = 0; i < vector_length(msg); i++) {
        if (0 == strncmp("\r\n\r\n", msg + i, 4)) {
            string_push_cstr(body, msg + i + 4);
        }
        char *s = "Content-Length: ";
        if (0 == strncmp(s, msg + i, strlen(s))) {
            length = atol(msg + i + strlen(s));
        }
    }
    vector_free(msg);
    int l = vector_length(body);
    while (l < length) {
        char buffer[1024] = {0};
        int r = SSL_read(ssl, buffer, 1023);
        if (r < 0) {
            if (SSL_get_error(ssl, r) != SSL_ERROR_WANT_READ) {
                fprintf(stderr, "Connection error\n");
                exit(1);
            }
        }
        else {
            l += r;
        }
        string_push_cstr(body, buffer);
    } 
    return body;
}

void write_file(const char *name, const char *content) {
    struct stat s;
    if (stat(name, &s) != -1) {
        if (s.st_mode & S_IFDIR) {
            fprintf(stderr, "Could not write file. '%s' is a directory\n", name);
            return;
        }
        if (s.st_mode & S_IFMT) {
            printf("File '%s' already exists. Do you want to overwrite it? (y/N) ", name);
            char c = fgetc(stdin);
            while (fgetc(stdin) != '\n');
            if (tolower(c) != 'y') {
                return;
            }
        }
    }
    FILE *fp = fopen(name, "w");
    if (fp == NULL) {
        char *err = string_format("Could not open file '%s' for write", name);
        perror(err);
        vector_free(err);
        exit(1);
    }
    if (fwrite(content, sizeof(char), vector_length(content), fp) < vector_length(content)) {
        fprintf(stderr, "Failed to write to file '%s'\n", name);
        exit(1);
    }

    fclose(fp);
}

void print_help(const char *name) {
    printf("BFUtils: CLI tool to add bfutils headers to your project\n");
    printf("usage: %s [-amvpt]\n", name);
    printf("Options:\n");
    printf("  --all      -a    Download all headers\n");
    printf("  --hashmap  -m    Download bfutils_hash.h header\n");
    printf("  --vector   -v    Download bfutils_vector.h header\n");
    printf("  --process  -p    Download bfutils_process.h header\n");
    printf("  --test     -t    Download bfutils_test.h header\n");
    printf("  --help     -h    Show this help menu and exit\n");
}

int main(int argc, char *argv[]) {
    struct option *longopts = (struct option[]) {
        (struct option) {.name = "all", .val = 'a', .flag = NULL, .has_arg = 0},
        (struct option) {.name = "hashmap", .val = 'm', .has_arg = 0, .flag = NULL},
        (struct option) {.name = "vector", .val = 'v', .has_arg = 0, .flag = NULL},
        (struct option) {.name = "process", .val = 'p', .has_arg = 0, .flag = NULL},
        (struct option) {.name = "help", .val = 'h', .has_arg = 0, .flag = NULL},
        (struct option) {.name = "test", .val = 't', .has_arg = 0, .flag = NULL},
        (struct option) {0},
    };
    int include_hashmap = 0;
    int include_vector = 0;
    int include_process = 0;
    int include_test = 0;

    char opt;
    do {
        opt = getopt_long(argc, argv, "amvpth", longopts, NULL); 
        switch (opt) {
            case -1:
                break;
            case 'h':
                print_help(argv[0]);
                exit(0);
                break;
            case 'm':
                include_hashmap = 1;
                break;
            case 'v':
                include_vector = 1;
                break;
            case 'p':
                include_process = 1;
                break;
            case 't':
                include_test = 1;
                break;
            case 'a':
                include_hashmap = 1;
                include_vector = 1;
                include_process = 1;
                include_test = 1;
                break;
            default:
                assert(0 && "Unknown option");
        }
    } while(opt > 0);

    if (!include_hashmap && !include_test && !include_vector && !include_process) {
        print_help(argv[0]);
        exit(0);
    }

    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    SSL *ssl = SSL_new(ctx);
    
    struct addrinfo hints = {.ai_family = AF_INET, .ai_socktype = SOCK_STREAM, .ai_protocol = 0};
    struct addrinfo *result;
    int s = getaddrinfo("raw.githubusercontent.com", "443", &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(1);
    }; 

    struct addrinfo *rp;
    int sock;
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (connect(sock, rp->ai_addr, rp->ai_addrlen) != -1)
            break;
        close(sock);
    }
    freeaddrinfo(result);

    if (rp == NULL) {
        fprintf(stderr, "Could not connect to raw.githubusercontent.com\n");
        exit(1);
    }
    SSL_set_fd(ssl, sock);
    SSL_connect(ssl);

    if (include_vector) {
        request_file(ssl, "bfutils_vector.h");
        char *content = get_file_response(ssl);
        write_file("bfutils_vector.h", content);
        vector_free(content);
    }
    if (include_hashmap) {
        request_file(ssl, "bfutils_hash.h");
        char *content = get_file_response(ssl);
        write_file("bfutils_hash.h", content);
        vector_free(content);
    }
    if (include_process) {
        request_file(ssl, "bfutils_process.h");
        char *content = get_file_response(ssl);
        write_file("bfutils_process.h", content);
        vector_free(content);
    }
    if (include_test) {
        request_file(ssl, "bfutils_test.h");
        char *content = get_file_response(ssl);
        write_file("bfutils_test.h", content);
        vector_free(content);
    }

    SSL_shutdown(ssl);
    close(sock);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
}
