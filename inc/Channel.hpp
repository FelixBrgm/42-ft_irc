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
		unsigned short					_user_limit;

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

		void	add_invite(const std::string& nickname);

		bool	is_invited(const std::string& nickname) const;
		bool	is_operator(const std::string& nickname) const;
		bool	is_banned(const std::string& nickname) const;

		// Getters
		std::string get_names_list();
		const std::vector<Client*>& get_clients() const;
        void set_password(const std::string &password);
        void add_invited_nick(const std::string &invited_nick);
        void set_is_invite_only(bool is_invite_only);
        void set_topic(const std::string &topic);
        void set_topic_restricted(bool is_topic_only_changeable_by_operators);
        void set_user_limit(unsigned short user_limit);
    
        // Getters
        std::string get_password() const;
        std::vector<std::string> get_invited_nicks() const;
        bool get_is_invite_only() const;
        std::string get_topic() const;
        bool get_is_topic_only_changeable_by_operators() const;
        unsigned short get_user_limit() const;
		std::string get_name() const;
		// Setter
};

#endif
