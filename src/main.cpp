/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zhma <zhma@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/04 16:44:33 by yihe              #+#    #+#             */
/*   Updated: 2026/08/09 19:02:06 by zhma             ###   ########.fr       */
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
		
		std::cout << "Port: " << servers[0].port << std::endl;	
		std::cout << "Root: " << servers[0].root << std::endl;
		std::cout << "Index: " << servers[0].index << std::endl;
		std::cout << "Configuration file parsed successfully" << std::endl;
		
		std::cout << "Number of servers: " << servers.size() << std::endl;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return (1);
	}
	
	std::vector<std::string> tokens;

// tokens.push_back("server");
// tokens.push_back("{");
// tokens.push_back("listen");
// tokens.push_back("8080");
// tokens.push_back(";");

// TokenStream stream(tokens);

// std::cout << stream.peek() << std::endl;

// stream.expect("server");
// stream.expect("{");
// stream.expect("listen");

// std::string port = stream.consume();

// stream.expect(";");

// std::cout << "Port = " << port << std::endl;
	

	Server	server;
	server.run();
	return (0);
}