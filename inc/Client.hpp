#ifndef CLIENT_HPP
#define CLIENT_HPP
#include <string>
#include <map>

#include "Constants.hpp"

class Channel;

enum ClientStatus {pass, nick, user, registered};

class Client
{


	private:
		int											_fd;

		std::string									_nickname;
		std::string									_username;
		std::string									_realname;
		std::string									_hostname;

		ClientStatus								_registration_status;

		std::map<std::string, Channel*>				_joined_channels;	
		Channel*									_active_channel;

		std::string									_in_buffer;
		std::string									_response_buffer;
		std::string									_out_buffer;

	public:
		Client(); // just for map default creation
		Client(int fd);
		~Client();

		Client(const Client& other);

		Client& operator= (const Client& other);

		void setup_client_data(std::string nickname, std::string username, std::string realname);

		void append_out_buffer(char* buffer);
		void append_in_buffer(char* buffer);

		bool is_incoming_msg_complete() const;
		bool is_incoming_msg_too_long() const;
		ClientStatus get_status() const;

		void clear_in_buffer();
		void clear_out_buffer();

		void disconnect();


};

#endif