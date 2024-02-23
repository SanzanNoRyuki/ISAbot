/**
 * @file dc_client.h
 * @author Roman Fulla <xfulla00>
 *
 * @brief Discord client header.
 */

#ifndef ISABOT_DC_CLIENT_H
#define ISABOT_DC_CLIENT_H

#include "isaexception.h"

#include <exception>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#include <string>
#include <vector>

/**
 * @brief DC_Client
 * Discord client.
 */
class DC_Client {
private:
    /**
     * @brief token
     * Discord bot authentication token.
     */
    std::string token;

    /**
     * @brief bio
     * SSL BIO(above connection BIO).
     */
    BIO *bio;

    /**
     * @brief ctx
     * SSL context.
     */
    SSL_CTX *ctx;

    /**
     * @brief get_ssl
     * Gets ssl from BIO.
     * @return SSL
     */
    static SSL *get_ssl(BIO *bio);

    /**
     * @brief receive_part
     * Receives part of the message.
     * @return part of the message
     */
    std::string receive_part(BIO *bio);

    /**
     * @brief split_headers
     * Splits headers.
     * @return split headers
     */
    static std::vector<std::string> split_headers(const std::string& text);
public:
    /**
     * @brief DC_Client
     * Constructor, creates client.
     */
    DC_Client(const std::string& token);

    /**
     * @brief ~DC_Client
     * Destructor, destroys client.
     */
    ~DC_Client();

    /**
     * @brief send_get
     * Sends GET message to discord.com.
     */
    void send_get(const std::string& destination);

    /**
     * @brief send_post
     * Sends POST message to discord.com.
     */
    void send_post(const std::string& destination, const std::string& payload);

    /**
     * @brief receive
     * Receives message from discord.com.
     * @return body of the message(head as a parameter)
     */
    std::string receive(std::string* head);
};

#endif