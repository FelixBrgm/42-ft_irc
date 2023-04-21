#ifndef CHANNEL_HPP
#define CHANNEL_HPP
#include <vector>
#include <set>
#include "Client.hpp"

class Channel
{
    private:
		std::string						_name;
		std::vector<Client*>			_channel_clients;
		std::vector<std::string>		_channel_operators;
		std::vector<std::string>		_banned_nicks;

		std::string						_password;
		std::vector<std::string>		_invited_nicks;
		bool							_is_invite_only;
		std::string						_topic;
		bool							_is_topic_only_changeable_by_operators;
		unsinged short					_user_limit;

    public:
        Channel();
        Channel(std::string name);
        ~Channel();

		void	add_client(Client *client);
		void	remove_client(Client *client);

		bool 	contains_client(Client* client) const;

		void	add_operator(const std::string& nickname);
		void	remove_operator(const std::string& nickname);

		void	add_ban(const std::string& nickname);
		void	remove_ban(const std::string& nickname);

		bool	is_invited(const std::string& nickname) const;
		bool	is_operator(const std::string& nickname) const;
		bool	is_banned(const std::string& nickname) const;

		// Getters
		std::string get_names_list();
		const std::vector<Client*>& get_clients() const;
		std::string get_topic() const;

		// Setter
		void set_topic(const std::string& topic);
		void set_invite(bool is_invite_only);
};

#endif
