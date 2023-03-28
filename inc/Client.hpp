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

		// Getters And Setters

		void append_response_buffer(std::string buffer);
		void append_out_buffer(std::string buffer);
		void append_in_buffer(char* buffer);

		void proceed_registration_status();
		std::string get_in_buffer();
		std::string get_nickname() const;
		void set_nickname(std::string& nickname);
		void set_username(std::string& username);
		void set_realname(std::string& realname);


		// Status Checkers
		bool is_incoming_msg_complete() const;
		bool is_incoming_msg_too_long() const;
		bool is_response_complete() const;
		ClientStatus get_status() const;




		void clear_msg_in_buffer();
		void clear_out_buffer();


		// Actions
		void disconnect();

		void send_out_buffer();
	
};

#endif