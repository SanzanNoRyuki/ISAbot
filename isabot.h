/**
 * @file isabot.h
 * @author Roman Fulla <xfulla00>
 *
 * @brief Isabot header file.
 */

#ifndef ISABOT_H
#define ISABOT_H

#include "dc_client.h"
#include "isaexception.h"

#include <exception>
#include <iostream>
#include <regex>
#include <vector>
#include <unistd.h>

/**
 * @brief argparse
 * Argument parser.
 * @return token
 */
std::string argparse(int argc, char *argv[], bool *verbose, bool *help);

/**
 * @brief out_help
 * Help function.
 */
void out_help();

/**
 * @brief isabot
 * Echoes user messages in the first isabot channel he finds.
 */
void isabot(const std::string& token, bool verbose);

/**
 * @brief check_head
 * Checks head of response.
 * @return flag if program should try again
 */
bool check_head(const std::string& head);

/**
 * @brief parse
 * Parses string according to given regex.
 * @return parsed string
 */
std::string parse(const std::string& source, const std::string& regex);

/**
 * @brief get_bot
 * Gets bot ID from the client.
 * @return bot ID
 */
ulong get_bot(DC_Client *client);

/**
 * @brief get_guilds
 * Gets IDs of guilds bot is a part of.
 * @return vector of guilds IDs
 */
std::vector<ulong> get_guilds(DC_Client *client);

/**
 * @brief get_channel
 * Gets ID of the first searched channel found in the given guilds.
 * @return channel ID
 */
ulong get_channel(DC_Client *client, const std::vector<ulong> &guilds, const std::string& searched, ulong &last_msg);

/**
 * @brief get_messages
 * Gets messages from the given channel after the last message.
 * @return channel messages
 */
std::vector<std::pair<std::string, std::string>> get_messages(DC_Client *client, ulong channel, ulong bot, ulong &last_msg);

/**
 * @brief echo
 * Echoes the given messages.
 */
void echo(DC_Client *client, const ulong channel, const std::vector<std::pair<std::string, std::string>>& messages, bool verbose);

#endif