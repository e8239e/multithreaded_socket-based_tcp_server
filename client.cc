#include <iostream>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

int main(int argc, char** argv) {
	if (argc < 4) {
		std::cerr << "not enough arguments" << std::endl;
		return 1;
	}

	std::string client_name = argv[1];
	int port = std::atoi(argv[2]);
	int connection_interval = std::atoi(argv[3]);

	int client_fd;

	struct sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);

	if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
		std::cerr << "invalid address" << std::endl;
		return 2;
	}

	while (1) {
		if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			std::cerr << "failed to create socket" << std::endl;
			return 3;
		}

		if (connect(client_fd, (struct sockaddr*)&serv_addr,
			    sizeof(serv_addr)) < 0) {
			std::cerr << "Connection Failed" << std::endl;
			return 4;
		}

		send(client_fd, client_name.c_str(), client_name.length(), 0);

		std::this_thread::sleep_for(
			std::chrono::seconds{ connection_interval });
		close(client_fd);
	}

	return 0;
}
