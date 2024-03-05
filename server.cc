#include <chrono>
#include <format>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <regex>
#include <stdexcept>
#include <string>
#include <thread>

#include <cstdlib>

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#define LOG_FILE "log.txt"

class Logger {
	private:
	std::ofstream _file;
	std::mutex _file_mutex;

	public:
	Logger(const char* log_path) {
		_file = std::ofstream{ log_path,
				       std::ios::out | std::ios::app };
		if (!_file) {
			throw std::runtime_error{ "failed to open log file" };
		}
	}
	void write(std::string msg) {
		std::scoped_lock lock{ _file_mutex };
		_file << msg;
		_file.flush();
	}
	~Logger() {
		std::scoped_lock lock{ _file_mutex };
		_file.flush();
		_file.close();
	}
};

class TCPServer {
	private:
	int server_fd;
	struct sockaddr_in address;
	socklen_t addrlen = sizeof(address);

	public:
	std::function<void(int)> connection_handler;

	TCPServer(int port) {
		if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			throw std::runtime_error{ "failed to create socket" };
		}
		int o = 1;
		if (setsockopt(server_fd, SOL_SOCKET,
			       SO_REUSEADDR | SO_REUSEPORT, &o, sizeof(o))) {
			throw std::runtime_error{
				"failed to set socket options"
			};
		}
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0
		address.sin_port = htons(port);
		if (bind(server_fd, (struct sockaddr*)&address, addrlen) < 0) {
			throw std::runtime_error{ "failed to bind" };
		}
		if (listen(server_fd, 10) < 0) {
			throw std::runtime_error{ "failed to listen" };
		}
	}

	void handle_connections(void) {
		int client_fd;
		while ((client_fd = accept(server_fd,
					   (struct sockaddr*)&address,
					   &addrlen))) {
			std::thread{ connection_handler, client_fd }.detach();
		}
	}
	~TCPServer(void) {
		close(server_fd);
	}
};

int main(int argc, char** argv) {
	if (argc < 2) {
		std::cerr << "not enough arguments" << std::endl;
		return 1;
	}

	int port = std::atoi(argv[1]);

	Logger log{ LOG_FILE };
	TCPServer server{ port };

	auto reg = std::regex{ "(\\.\\d{3})\\d*" };
	server.connection_handler = [&log, reg](int client_socket_fd)
	{
		auto buf = std::make_unique<char[]>(2048);
		std::string log_line;
		while (read(client_socket_fd, buf.get(), 2048 - 1) > 0) {
			log_line = std::regex_replace(
				std::format("[{:%Y-%m-%d %H:%M:%S}] {} \n",
					    std::chrono::system_clock{}.now(),
					    buf.get()),
				std::regex{ "(\\.\\d{3})\\d*" }, "$1");
			log.write(log_line);
			std::cerr << log_line;
		}
		close(client_socket_fd);
	};

	server.handle_connections();

	return 0;
}
