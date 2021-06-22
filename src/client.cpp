#include <arpa/inet.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>


#define PORT 1234

int main()
{
	int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(socket_fd == -1)
	{
		throw std::runtime_error("Could not create a socket");
	}

	sockaddr_in server_address = {
		.sin_family = AF_INET,
		.sin_port	= PORT,
	};

	int connect_result = connect(socket_fd, (sockaddr *) &server_address, sizeof(server_address));
	if(connect_result == -1)
	{
		throw std::runtime_error("Could not connect to server");
	}

	char servbuf[1920 * 3];

	std::ofstream file("tmp.ppm", std::ios::out | std::ios::binary);
	file << "P6\n"
		 << 1920 << "\n"
		 << 1080 << "\n"
		 << 255 << "\n";

	int height = 0;

	while(1)
	{
		
		/*if(height == 0)
		{
			file.open("tmp.ppm", std::ios::out | std::ios::binary);
			file << "P6\n"
				 << 1920 << "\n"
				 << 1080 << "\n"
				 << 255 << "\n";
		}*/

		int server_read = read(socket_fd, servbuf, 1920 * 3);

		// Debugging: write to ppm
		{

			uint32_t *row = (uint32_t *) servbuf;
			printf("row: %s\n", (char*) row);

			for(uint32_t x = 0; x < 1080; x++)
			{
				file.write((char *) row, 3);
				row++;
			}

			height++;

			char write_packet[1];
			write(socket_fd, write_packet, 1);
		}

		if(height == 1080)
		{
			file.close();
			printf("height = 1080\n");
			height = 0;
		}
	}

	/*send(socket_fd, "fuck", 4, 0);
	printf("msg sent\n");
	
	char servbuf[1024];
	int server_read = read(socket_fd, servbuf, 1024);
	printf("Message from server: %s\n", servbuf);*/

	return 0;
}