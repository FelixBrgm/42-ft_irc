#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <map>

#include "Constants.hpp"

class Channel;

enum ClientStatus { pass, nick, user, registered };

class Client {
private:
	int			_fd;

	std::string						_nickname;
	std::string						_username;
	std::string						_realname;

	ClientStatus					_registration_status;

	std::map<std::string, Channel*>	_joined_channels;
	Channel*						_active_channel;

	std::string						_in_buffer;
	std::string						_response_buffer;
	std::string						_out_buffer;

public:
	Client(); // just for map default creation
	Client(int fd);
	~Client();

	Client(const Client& other);
	Client& operator= (const Client& other);

	// Getters
	int								get_fd() const;

	std::string						get_nickname() const;
	std::string						get_username() const;
	std::string						get_realname() const;

	ClientStatus					get_status() const;

	std::map<std::string, Channel*> get_joined_channels();

	std::string						get_in_buffer();

	// Setters
	void							set_nickname(std::string& nickname);
	void							set_username(std::string& username);
	void							set_realname(std::string& realname);

	// Buffer Operations
	void							append_response_buffer(std::string buffer);
	void							append_out_buffer(std::string buffer);
	void							append_in_buffer(char* buffer);
	void							cut_msg_in_buffer();
	void							clear_out_buffer();

	// Status Checkers
	bool							is_incoming_msg_complete() const;
	bool							is_incoming_msg_too_long() const;
	bool							is_response_complete() const;

	// Actions
	void							disconnect();
	void							join_channel(std::string channel_name, Channel *channel_to_join);
	short							send_out_buffer();

	// Registration
	void							proceed_registration_status();

};

#endif
