/**
 * @file isaexception.h
 * @author Roman Fulla <xfulla00>
 *
 * @brief ISAexception definition.
 */

#ifndef ISABOT_ISAEXCEPTION_H
#define ISABOT_ISAEXCEPTION_H

#include <exception>
#include <string>

/**
 * @brief ISAexception
 * Exception for catching problems unique to isabot.
 */
class ISAexception : public std::exception {
public:
    int ret;
    std::string msg;
    ISAexception(const char *msg, int code) : ret(code), msg(msg) {}
};

#endif

