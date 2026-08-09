/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zhma <zhma@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/04 16:44:33 by yihe              #+#    #+#             */
/*   Updated: 2026/08/07 16:46:50 by zhma             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "ConfigParser.hpp"
#include "ServerConfig.hpp"
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

	ConfigParser parser;
	std::vector<ServerConfig> servers;

	servers = parser.parse(argv[1]);
	std::cout << "Port: " << servers[0].port << std::endl;	
	std::cout << "Root: " << servers[0].root << std::endl;
	std::cout << "Index: " << servers[0].index << std::endl;
	std::cout << "Configuration file parsed successfully" << std::endl;
	

	Server	server;
	server.run();
	return (0);
}