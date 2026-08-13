/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zhma <zhma@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/04 16:44:33 by yihe              #+#    #+#             */
/*   Updated: 2026/08/13 16:44:05 by zhma             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "ConfigParser.hpp"
#include "ServerConfig.hpp"
#include "TokenStream.hpp"
#include "Tokenizer.hpp"

#include <iostream>

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		std::cerr << "Usage: ./webserv <configuration_file>"
				  << std::endl;
		return (1);
	}

	std::cout << argv[1] << std::endl;
	// create Config
	
	try
	{
		ConfigParser parser;
		std::vector<ServerConfig> servers;
		servers = parser.parse(argv[1]);
		// std::cout << "Port: " << servers[0].port << std::endl;	
		// std::cout << "Root: " << servers[0].root << std::endl;
		// std::cout << "Index: " << servers[0].index << std::endl;
		// std::cout << "Configuration file parsed successfully" << std::endl;
		
		std::cout << "Number of servers: " << servers.size() << std::endl;
		
			for (size_t i = 0; i < servers.size(); i++)
		{
			std::cout << servers[i].port << std::endl;
			printf("location size: %zu\n", servers[i].locations.size());
			for (size_t j = 0;
					j < servers[i].locations.size();
					j++)
			{
				std::cout
					<< servers[i].locations[j].path
					<< std::endl;

				std::cout
					<< servers[i].locations[j].root
					<< std::endl;
				// std::cout
				// 	<< servers[i].locations[j].methods
				// 	<< std::endl;
				// std::cout
				// 	<< servers[i].locations[j].methods
				// 	<< std::endl;
				for (size_t k = 0;
						k < servers[i].locations[j].methods.size();
						k++)
				{
					std::cout
						<< servers[i].locations[j].methods[k]
						<< std::endl;
				}
			}
		}
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return (1);
	}
	
	std::vector<std::string> tokens;


	Server	server;
	server.run();
	return (0);
}