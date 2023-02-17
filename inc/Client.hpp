#ifndef CLIENT_HPP
#define CLIENT_HPP
#include <string>
#include <map>

class Channel;

class Client
{
	private:
		int											_fd;

		std::string									_nickname;
		std::string									_username;
		std::string									_realname;

		bool										_is_registered;

		static std::map<std::string, Channel>*		_server_channels;
		Channel*									_active_channel;

		std::string									_in_buffer;
		std::string									_out_buffer;

	public:
		Client();
		Client(int fd);
		~Client();

		Client(const Client& other);

		Client& operator= (const Client& other);

		void setup_client_data(std::string nickname, std::string username, std::string realname);

		void append_out_buffer(std::string buffer);
		void append_in_buffer(std::string buffer);

		bool is_incoming_msg_complete() const;
		bool is_registered() const;


		void clear_in_buffer();
		void clear_out_buffer();
};

#endif