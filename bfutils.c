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
    printf("usage: %s [-amvptbh]\n", name);
    printf("Options:\n");
    printf("  --all      -a    Download all headers\n");
    printf("  --hashmap  -m    Download bfutils_hash.h header\n");
    printf("  --vector   -v    Download bfutils_vector.h header\n");
    printf("  --process  -p    Download bfutils_process.h header\n");
    printf("  --test     -t    Download bfutils_test.h header\n");
    printf("  --build    -b    Download bfutils_build.h header\n");
    printf("  --help     -h    Show this help menu and exit\n");
}

#define HEADERS \
    X(HASHMAP, "bfutils_hash.h", 'm', "hashmap") \
    X(VECTOR, "bfutils_vector.h", 'v', "vector") \
    X(PROCESS, "bfutils_process.h", 'p', "process") \
    X(TEST, "bfutils_test.h", 't', "test") \
    X(BUILD, "bfutils_build.h", 'b', "build")

enum Header {
#define X(name, file, o, opt) HEADER_##name, 
    HEADERS
#undef X
    HEADER_COUNT
};

const char *header_files[HEADER_COUNT] = {
#define X(name, file, o, opt) [HEADER_##name] = file, 
    HEADERS
#undef X
};

int main(int argc, char *argv[]) {
    struct option *longopts = (struct option[]) {
        (struct option) {.name = "all", .val = 'a', .has_arg = 0, .flag = NULL},
        #define X(n, f, o, opt) \
        (struct option) {.name = opt, .val = o, .has_arg = 0, .flag = NULL},
            HEADERS
        #undef X
        (struct option) {.name = "help", .val = 'h', .has_arg = 0, .flag = NULL},
        (struct option) {0},
    };
    int include_header[HEADER_COUNT] = {0};
    char options[HEADER_COUNT + 3] = {
        'a',
        #define X(n, f, o, opt) o,
            HEADERS
        #undef X
        'h',
        '\0',
    };

    char opt;
    do {
        opt = getopt_long(argc, argv, options, longopts, NULL); 
        switch (opt) {
            case -1:
                break;
            case 'h':
                print_help(argv[0]);
                exit(0);
                break;
            case 'm':
                include_header[HEADER_HASHMAP] = 1;
                break;
            case 'v':
                include_header[HEADER_VECTOR] = 1;
                break;
            case 'p':
                include_header[HEADER_PROCESS] = 1;
                break;
            case 't':
                include_header[HEADER_TEST] = 1;
                break;
            case 'b':
                include_header[HEADER_BUILD] = 1;
                break;
            case 'a':
                for(int i = 0; i < HEADER_COUNT; i++) {
                    include_header[i] = 1;
                }
                break;
            default:
                assert(0 && "Unknown option");
        }
    } while(opt > 0);

    int need_help = 1;
    for (int i = 0; i < HEADER_COUNT; i++) {
        if(include_header[i]) {
            need_help = 0;
            break;
        }
    }
    if (need_help) {
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

    for(int i = 0; i < HEADER_COUNT; i++) {
        if (include_header[i]) {
            request_file(ssl, header_files[i]);
            char *content = get_file_response(ssl);
            write_file(header_files[i], content);
            vector_free(content);
        }
    }

    SSL_shutdown(ssl);
    close(sock);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
}
