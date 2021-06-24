#include <arpa/inet.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>


#define PORT 1234


struct Client
{
	int socket_fd;

	Client()
	{
		socket_fd = socket(AF_INET, SOCK_STREAM, 0);
		if(socket_fd == -1)
		{
			throw std::runtime_error("Could not create a socket");
		}
	}

	void connect_to_server(int port)
	{
		sockaddr_in server_address = {
			.sin_family = AF_INET,
			.sin_port	= static_cast<in_port_t>(port),
		};


		int connect_result = connect(socket_fd, (sockaddr *) &server_address, sizeof(server_address));
		if(connect_result == -1)
		{
			throw std::runtime_error("Could not connect to server");
		}
	}

	void run()
	{
		uint32_t servbuf[1920 * 3];

		std::ofstream file("tmp.ppm", std::ios::out | std::ios::binary);
		file << "P6\n"
			<< 1920 << "\n"
			<< 1080 << "\n"
			<< 255 << "\n";

		int height = 0;

		while(1)
		{
			int server_read = read(socket_fd, servbuf, 1920 * 3);

			if(server_read != -1)
			{
				uint32_t *row = servbuf;
				char *rowchar = (char *) row;

				for(uint8_t i = 0; i < 3; i++)
				{
					printf("row: %c\n", rowchar[i]);
				}

				for(uint32_t x = 0; x < 1920; x++)
				{
					file.write((char *) row, 3);
					row++;
				}

				height++;

				write(socket_fd, "linedone", 8);
			}

			else
			{
				printf("Nothing to read\n");
			}

			if(height == 1920)
			{
				//file.close();
				printf("height = 1080\n");
				height = 0;
			}
		}
	}
};



int main()
{
	Client client = Client();
	client.connect_to_server(PORT);
	client.run();

	return 0;
}