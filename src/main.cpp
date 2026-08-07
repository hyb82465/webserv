/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zhma <zhma@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/04 16:44:33 by yihe              #+#    #+#             */
/*   Updated: 2026/08/07 11:56:56 by zhma             ###   ########.fr       */
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
	ServerConfig config;

	config = parser.parse(argv[1]);
	std::cout << "Port: " << config.port << std::endl;	
	std::cout << "Root: " << config.root << std::endl;
	std::cout << "Index: " << config.index << std::endl;
	std::cout << "Configuration file parsed successfully" << std::endl;
	

	Server	server;
	server.run();
	return (0);
}
