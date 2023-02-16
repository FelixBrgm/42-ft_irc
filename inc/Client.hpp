#ifndef CLIENT_HPP
#define CLIENT_HPP
#include <string>
class Client
{
	private:
		std::string		_nickname;
		std::string		_username;
		std::string		_realname;
		int				_fd;
		std::string		_in_buffer;
		std::string		_out_buffer;

	public:
		Client(int fd);
		~Client();

		Client(const Client& other);

		Client& operator= (const Client& other);

		void set_user_data(std::string nickname, std::string username, std::string realname);
		void append_buffer(std::string buffer);
		bool is_incoming_msg_complete() const;
		
};

#endif