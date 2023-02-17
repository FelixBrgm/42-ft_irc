#ifndef CHANNEL_HPP
#define CHANNEL_HPP
#include <vector>

#include "Client.hpp"

class Channel
{
    private:
        std::vector<Client>     _channel_clients;
        std::vector<Client>     _channel_operators;

    public:
        Channel();
        ~Channel();
};

#endif