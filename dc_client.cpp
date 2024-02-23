/**
 * @file dc_client.cpp
 * @author Roman Fulla <xfulla00>
 *
 * @brief Discord client class.
 */

#include "dc_client.h"
#include "isaexception.h"

#include <exception>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#include <string>
#include <vector>

/* Consturctor */
DC_Client::DC_Client(const std::string& token) : token(token) {
    SSL_library_init();

    ctx = SSL_CTX_new(TLSv1_2_client_method());
    if (SSL_CTX_set_default_verify_paths(ctx) != 1) throw ISAexception("Couldn't set up a trust store.", 10);

    auto connect_bio = BIO_new_connect("discord.com:443");
    if (connect_bio == nullptr) throw ISAexception("Error in BIO_new_connect.", 20);
    if (BIO_do_connect(connect_bio) <= 0) throw ISAexception("Error in BIO_do_connect.", 21);

    bio = BIO_new_ssl(ctx, 1);
    bio = BIO_push(bio, connect_bio);

    std::string hostname = "discord.com";
    SSL_set_tlsext_host_name(get_ssl(bio), hostname.data());
    if (BIO_do_handshake(bio) <= 0) throw ISAexception("Error in BIO_do_handshake.", 30);

    if (SSL_get_verify_result(get_ssl(bio)) != X509_V_OK) throw ISAexception("Certificate verification error.", 40);

    X509 *cert = SSL_get_peer_certificate(get_ssl(bio));
    if (cert == nullptr) throw ISAexception("No certificate was presented by the server.", 41);
    if (X509_check_host(cert, hostname.data(), hostname.size(), 0, nullptr) != 1) throw ISAexception("Hostnames are not matching.", 42);
}

/* Destructor */
DC_Client::~DC_Client() {
    BIO_free_all(bio);
    SSL_CTX_free(ctx);
}

/* Gets ssl from the BIO */
SSL *DC_Client::get_ssl(BIO *bio) {
    SSL *ssl = nullptr;
    BIO_get_ssl(bio, &ssl);
    if (ssl == nullptr) throw ISAexception("Error in BIO_get_ssl.", 50);
    return ssl;
}

/* Sends get message to discord.com */
void DC_Client::send_get(const std::string& destination) {
    std::string request = "GET " + destination + " HTTP/1.1\r\n";
    request += "Host: discord.com\r\n";
    request += "Authorization: Bot " + token + "\r\n";
    request += "\r\n";

    BIO_write(bio, request.data(), request.size());
    BIO_flush(bio);
}

/* Sends post message to discord.com */
void DC_Client::send_post(const std::string& destination, const std::string& payload) {
        std::string request = "POST " + destination + " HTTP/1.1\r\n";
        request += "Host: discord.com\r\n";
        request += "Authorization: Bot " + token + "\r\n";
        request += "Content-Length: " + std::to_string(payload.size() + 15) + "\r\n";
        request += "Content-Type: application/json\r\n";
        request += "\r\n";
        request += "{\"content\": \"" + payload + "\"}\r\n";
        request += "\r\n";

        BIO_write(bio, request.data(), request.size());
        BIO_flush(bio);
}

/* Receives message from discord.com */
std::string DC_Client::receive(std::string* head) {
    std::string headers = receive_part(bio);
    char *end_of_headers = strstr(&headers[0], "\r\n\r\n");
    while (end_of_headers == nullptr) {
        headers += receive_part(bio);
        end_of_headers = strstr(&headers[0], "\r\n\r\n");
    }

    std::string body = std::string(end_of_headers+4, &headers[headers.size()]);
    headers.resize(end_of_headers + 2 - &headers[0]);

    bool chunked = false;
    size_t content_length = 0;
    for (const std::string& line : split_headers(headers)) {
        if (const char *colon = strchr(line.c_str(), ':')) {
            auto header_name = std::string(&line[0], colon);
            if (header_name == "Transfer-Encoding" && std::string(colon+2) == "chunked") {
                chunked = true;
                break;
            }
            else if (header_name == "Content-Length") {
                content_length = std::stoul(colon+1);
                break;
            }
        }
    }

    if (chunked) {
        while (true) {
            auto part = receive_part(bio);
            if (part == "0\r\n\r\n") break;
            body += part;
        }
    }
    else {
        while (body.size() < content_length) {
            body += receive_part(bio);
        }
    }

    // return headers + "\r\n" + body;
    *head = headers;
    return body;
}

/* Receives part of the message */
std::string DC_Client::receive_part(BIO *bio) {
    char buffer[1024];
    int len = BIO_read(bio, buffer, sizeof(buffer));
    if (len < 0) throw ISAexception("Error in BIO_read", 100);
    else if (len > 0) return std::string(buffer, len);
    else if (BIO_should_retry(bio)) return receive_part(bio);
    else throw ISAexception("Empty BIO_read.", 101);
}

/* Splits headers */
std::vector<std::string> DC_Client::split_headers(const std::string& text) {
    std::vector<std::string> lines;
    const char *start = text.c_str();
    while (const char *end = strstr(start, "\r\n")) {
        lines.push_back(std::string(start, end));
        start = end + 2;
    }
    return lines;
}
