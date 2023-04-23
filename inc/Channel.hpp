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
		unsigned short					_user_limit;

		bool							_is_invite_only;
		std::vector<std::string>		_invited_nicks;

		bool							_is_topic_only_changeable_by_operators;
		std::string						_topic;

	public:
		Channel();
		Channel(std::string name);
		~Channel();

		// Client 
		void						add_client(Client *client);
		void						remove_client(Client *client);
		bool						contains_client(Client* client) const;

		// Operator
		void						add_operator(const std::string& nickname);
		void						remove_operator(const std::string& nickname);
		bool						is_operator(const std::string& nickname) const;

		// Ban
		void						add_ban(const std::string& nickname);
		void						remove_ban(const std::string& nickname);
		bool						is_banned(const std::string& nickname) const;

		// Invite
		void						add_invite(const std::string& nickname);
		bool						is_invited(const std::string& nickname) const;

		// Limit
		bool						is_full() const;

		// Getters
		std::string					get_name() const;
		const std::vector<Client*>&	get_clients() const;
		const size_t				get_clients_count() const;
		std::string					get_names_list();

		std::string					get_password() const;
		unsigned short				get_user_limit() const;

		bool						get_is_invite_only() const;
		std::vector<std::string>	get_invited_nicks() const;
		
		bool						get_is_topic_only_changeable_by_operators() const;
		std::string					get_topic() const;

		// Getters
		void						set_password(const std::string &password);
		void						set_user_limit(unsigned short user_limit);
		void						set_is_invite_only(bool is_invite_only);
		void						set_topic_restricted(bool is_topic_only_changeable_by_operators);
		void						set_topic(const std::string &topic);
	
};

#endif
