/*
    Philip Salire
    psalire@ucsc.edu 1477868
    CMPE 156/L: Final Project: Chat
*/

#ifndef _USER_H_
#define _USER_H_

#include <string>
#include <regex>
#include <iostream>

class User {
    std::string name;
    public:
        User() {};
        User(const std::string user_name) : name(user_name) {};
        void setName(std::string user_name);
        std::string getName() {return name;}
};

#endif
