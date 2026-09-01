/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zhma <zhma@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/04 16:44:33 by yihe              #+#    #+#             */
/*   Updated: 2026/08/16 09:42:07 by zhma             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "ConfigParser.hpp"
#include "ConfigValidator.hpp"
#include "ServerConfig.hpp"
#include "TokenStream.hpp"
#include "Tokenizer.hpp"
#include "Signal.hpp"

#include <iostream>
#include <stdexcept>
#include <vector>

volatile sig_atomic_t g_running = 1;

void handleSignal(int signal)
{
	(void)signal;
	g_running = 0;
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		std::cerr << "Usage: ./webserv <configuration_file>"
				  << std::endl;
		return (1);
	}
	try
	{
		std::signal(SIGINT, handleSignal);

		ConfigParser parser;
		std::vector<ServerConfig> servers;
		servers = parser.parse(argv[1]);

		ConfigValidator validator;
		validator.validate(servers);

		Server server(servers);
		server.run();
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return (1);
	}
	return (0);
}
