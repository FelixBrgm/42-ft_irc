#ifndef SERVER_HPP
#define SERVER_HPP
#include <string>
#include <poll.h>


#include <arpa/inet.h>

#define MAX_CLIENTS_POLLFDS 1024

class Server
{
private:
	int						_port;
	std::string				_password;
	struct pollfd			_fds[MAX_CLIENTS_POLLFDS];
	unsigned short			_nfds;
	struct sockaddr_in		_listener_socket_addr;
	int						_listenerfd;
public:
	Server(int port, std::string password);
	~Server();
	void	start();
	void	loop();

private:
	void	_accept();
	void	_handle(std::string text);

};



#endif