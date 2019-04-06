#include "user.h"

void User::setName(const std::string user_name) {
    /* Check if valid username */
    if (!std::regex_match(user_name, std::regex("^[0-9a-z]+$", std::regex::icase))) {
        std::cout << "Error: Invalid username \"" << user_name
                  << "\". Username must consist only of [a-zA-z0-9]. Exiting.\n";
        exit(-1);
    }
    name = user_name;
}
