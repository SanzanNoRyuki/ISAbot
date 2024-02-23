/**
 * @file isabot.cpp
 * @author Roman Fulla <xfulla00>
 *
 * @brief Isabot source file.
 */

#include "isabot.h"
#include "dc_client.h"
#include "isaexception.h"

#include <exception>
#include <iostream>
#include <regex>
#include <vector>
#include <unistd.h>

/* Main program */
int main(int argc, char *argv[]) {
    bool verbose, help;
    std::string token = argparse(argc, argv, &verbose, &help);
    if (help) out_help();

    while (true) {
        try {
            isabot(token, verbose);
        }
        catch (ISAexception &e) {
            int err_cnt = 0;
            if (e.ret == 100 || e.ret == 101 || e.ret == 240) {
                std::cerr << e.msg << "..retrying(potential data loss)" << std::endl;
                continue;
            }
            else {
                err_cnt++;
                if (err_cnt == 3) {
                    std::cerr << e.msg << " Fatal error." << std::endl;
                    return e.ret;
                }

                std::cerr << e.msg << "..retrying(potential data loss)" << std::endl;
                continue;
            }
        }
    }
}

/* Argument parser */
std::string argparse(int argc, char *argv[], bool *verbose, bool *help) {
    std::string token = "";
    *verbose = false;
    *help = false;

    if (argc == 3 && std::string(argv[1]) == "-t") {
        token = argv[2];
    } else if (argc == 4 && std::string(argv[1]) == "-t" && std::string(argv[3]) == "-v") {
        *verbose = true;
        token = argv[2];
    } else if (argc == 4 && std::string(argv[1]) == "-t" && std::string(argv[3]) == "--verbose") {
        *verbose = true;
        token = argv[2];
    } else if (argc == 4 && std::string(argv[1]) == "-v" && std::string(argv[2]) == "-t") {
        *verbose = true;
        token = argv[3];
    } else if (argc == 4 && std::string(argv[1]) == "--verbose" && std::string(argv[2]) == "-t") {
        *verbose = true;
        token = argv[3];
    } else *help = true;  // Any other combination results with calling help.

    return token;
}

/* Help function */
void out_help() {
    std::cout << "This is a bot that echoes user messages on the isa-bot discord channel."            << std::endl;
    std::cout << "isabot [-h|--help] [-v|--verbose] -t <bot_access_token>"                            << std::endl;
    std::cout << "----------------------------------------------------------------------------------" << std::endl;
    std::cout << "-h | --help           : Shows this."                                                << std::endl;
    std::cout << "-v | --verbose        : Messages bot reacted to are output on the standard output." << std::endl;
    std::cout << "-t <bot_access_token> : Authentication token needed to connect to a bot."           << std::endl;

    exit(0);
}

/* Isabot program */
void isabot(const std::string& token, bool verbose) {
    DC_Client client(token);
    ulong last_msg;
    ulong bot = get_bot(&client);
    std::vector<ulong> guilds = get_guilds(&client);
    ulong channel = get_channel (&client, guilds, "isa-bot", last_msg);

    while (true) {
        usleep(1000000);
        std::vector<std::pair<std::string, std::string>> messages = get_messages(&client, channel, bot, last_msg);
        echo(&client, channel, messages, verbose);
    }
}

/* Checks head of response */
bool check_head(const std::string& head) {
    std::size_t pos = head.find("HTTP/1.1 ");
    if (pos == std::string::npos) throw ISAexception("Wrong HTTP response.", 200);

    char *ptr;
    int code = strtol((head.substr(pos + 9, 3)).data(), &ptr, 10);

    switch (code) {
        case 200:
            return false;
        case 204:
            usleep(2000000);
            return true;
        case 400:
            throw ISAexception("Bad request HTTP response.", 220);
        case 401:
            throw ISAexception("Unauthorized HTTP response.", 221);
        case 403:
            throw ISAexception("Forbidden HTTP response.", 223);
        case 404:
            throw ISAexception("Not found HTTP response.", 224);
        case 405:
            throw ISAexception("Method not allowed HTTP response.", 225);
        case 429:
            usleep(2000000);
            return true;
        case 502:
            throw ISAexception("Gateway unavailable HTTP response.", 226);
        default:
            throw ISAexception("Unexpected HTTP response.", 230);
    }
}

/* Get parsed part from the string */
std::string parse(const std::string& source, const std::string& pattern) {
    std::smatch match;
    std::regex rgx(pattern);

    if (std::regex_search(source, match, rgx)) return match[0];
    else throw ISAexception("Regex has failed (again).", 240);
}

/* Gets bot ID from the client */
ulong get_bot(DC_Client *client) {
    std::string head, body;
    bool retry = false;
    do {
        client->send_get("/api/users/@me");
        body = client->receive(&head);
        retry = check_head(head);
    } while (retry);

    std::string bot = parse(body, "\"id\": \"[0-9]+");
    bot = bot.substr(7);
    char *ptr;
    return strtoul(bot.data(), &ptr, 10);
}

/* Gets IDs of guilds bot is a part of */
std::vector<ulong> get_guilds(DC_Client *client) {
    std::string head, body;
    bool retry = false;
    do {
        client->send_get("/api/users/@me/guilds");
        body = client->receive(&head);
        retry = check_head(head);
    } while (retry);

    std::vector<std::string> guilds;
    std::vector<ulong> guilds_ids;

    while (true) {
        std::size_t start = body.find("{");
        if (start == std::string::npos) break;
        std::size_t end = body.find("}");
        if (end == std::string::npos) break;

        std::string guild_str = body.substr(start + 1, end - start - 1);
        guilds.push_back(guild_str);
        body.erase(start, end - start + 1);
    }

    if (guilds.empty()) throw ISAexception("Bot is not a member of any guild.", 250);

    for (auto const& guild : guilds) {
        std::string guild_id = parse(guild, "\"id\": \"[0-9]+");
        guild_id = guild_id.substr(7);
        char *ptr;
        guilds_ids.push_back(strtoul(guild_id.data(), &ptr, 10));
    }

    return guilds_ids;
}

/* Gets ID of the first wanted channel in the given guilds */
ulong get_channel(DC_Client *client, const std::vector<ulong> &guilds, const std::string& searched, ulong &last_msg) {
    ulong channel_id = 0;
    last_msg = 0;

    for (auto const& guild : guilds) {
        std::string head, body;
        bool retry = false;
        do {
            client->send_get("/api/guilds/" + std::to_string(guild) + "/channels");
            body = client->receive(&head);
            retry = check_head(head);
        } while (retry);

        /* Permission overwrites cleanup */
        while (true) {
            std::size_t start = body.find("\"permission_overwrites\": [");
            if (start == std::string::npos) break;
            std::size_t end = body.find("]");
            if (end == std::string::npos) break;

            body.erase(start, end - start + 1);
        }

        std::vector<std::string> channels;
        while (true) {
            std::size_t start = body.find("{");
            if (start == std::string::npos) break;
            std::size_t end = body.find("}");
            if (end == std::string::npos) break;

            std::string channel_str = body.substr(start + 1, end - start - 1);
            channels.push_back(channel_str);
            body.erase(start, end - start + 1);
        }

        for (auto const& channel : channels) {
            if (channel.find("\"name\": \"" + searched) == std::string::npos) continue;

            char *ptr;

            std::string id = parse(channel, "\"id\": \"[0-9]+");
            id = id.substr(7);
            channel_id = strtoul(id.data(), &ptr, 10);

            std::string msg = parse(channel, "\"last_message_id\": \"[0-9]+");
            msg = msg.substr(20);
            last_msg = strtoul(msg.data(), &ptr, 10);

            return channel_id;
        }
    }

    throw ISAexception("No channel named isa-bot was found in guilds bot is a part of.", 251);
}

/* Gets messages from the given channel after the last message */
std::vector<std::pair<std::string, std::string>> get_messages(DC_Client *client, ulong channel, ulong bot, ulong &last_msg) {
    std::string head, body;
    bool retry = false;
    do {
        client->send_get("/api/channels/" + std::to_string(channel) + "/messages?after=" + std::to_string(last_msg));
        body = client->receive(&head);
        retry = check_head(head);
        if (body == "[]") retry = true;
    } while (retry);

    /* Updates last message */
    std::string last_msg_str = parse(body, "\"id\": \"[0-9]+");
    last_msg_str = last_msg_str.substr(7);
    char *ptr;
    last_msg = strtoul(last_msg_str.data(), &ptr, 10);

    std::vector<std::string> msg_vect;
    while (true) {
        std::size_t start = body.find("{");
        if (start == std::string::npos) break;
        std::size_t end = body.find("}, {");
        if (end == std::string::npos) {
            std::size_t end = body.find("}]");
            if (end == std::string::npos) break;
        }

        std::string message_str = body.substr(start + 1, end - start - 1);
        msg_vect.push_back(message_str);
        body.erase(start, end - start + 1);
    }

    std::vector<std::pair<std::string, std::string>> messages;
    for (auto const& message : msg_vect) {
        std::pair<std::string, std::string> msg;

        std::string user_id_str = parse(message, "\"author\": .\"id\": \"[0-9]+");
        user_id_str = user_id_str.substr(18);
        ulong user_id = strtoul(user_id_str.data(), &ptr, 10);

        /* Parsing own message */
        if (user_id == bot) continue;

        std::string username = parse(message, "\"username\": \".*?\"");
        username = username.substr(13);
        username = username.substr(0, username.size() - 1);
        msg.first = username;

        /* Parsing message of a different bot */
        std::size_t is_bot = username.find("bot");
        if (is_bot != std::string::npos) continue;

        std::string content = parse(message, "\"content\": \".*?\", \"channel_id\": \"");
        content = content.substr(12);
        content = content.substr(0, content.size() - 18);
        msg.second = content;

        messages.push_back(msg);
    }

    std::reverse(messages.begin(),messages.end());
    return messages;
}

/* Echoes the given messages */
void echo(DC_Client *client, const ulong channel, const std::vector<std::pair<std::string, std::string>>& messages, bool verbose) {
    for (auto const& msg_pair : messages) {
       std::string message = "echo: " + msg_pair.first + " - " + msg_pair.second;

       std::string head, body;
       bool retry = false;
       do {
           client->send_post("/api/channels/" + std::to_string(channel) +"/messages", message);
           body = client->receive(&head);
           retry = check_head(head);
       } while (retry);

       if (verbose) std::cout << message << std::endl;
    }
}