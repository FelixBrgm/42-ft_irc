#ifndef CHANNEL_HPP
#define CHANNEL_HPP
#include <vector>

#include "Client.hpp"

class Channel
{
    private:
		std::string					_name;
		bool						_is_joinable;
        std::vector<Client*>		_channel_clients;
        std::vector<std::string>	_channel_operators;

    public:
        Channel();
        ~Channel();

		void	remove_client(Client *client);		
};


#endif
