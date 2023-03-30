#ifndef CHANNEL_HPP
#define CHANNEL_HPP
#include <vector>
#include <set>
#include "Client.hpp"

class Channel
{
    private:
		std::string					_name;
		std::vector<Client*>		_channel_clients;
		std::set<std::string>		_channel_operators;
		bool						_is_moderated;
		std::set<std::string>		_banned_nicks;

    public:
        Channel();
        Channel(std::string name);
        ~Channel();

		void	remove_client(Client *client);
		void	add_client(Client *client);

		std::string get_names_list();
		const std::vector<Client*>& get_clients() const;
		bool contains_client(Client* client) const;

		void add_operator(const std::string& nickname);
		void remove_operator(const std::string& nickname);
		bool is_operator(const std::string& nickname) const;


		void set_moderated(bool moderated);
		void add_ban(const std::string& nickname);
		void remove_ban(const std::string& nickname);
		bool is_banned(const std::string& nickname) const;
};




#endif
