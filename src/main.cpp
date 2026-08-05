/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yihe <yihe@learner.42.tech>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/04 16:44:33 by yihe              #+#    #+#             */
/*   Updated: 2026/08/04 17:04:02 by yihe             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
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
	std::cout << "Configuration loaded" << std::endl;

	Server	server;
	server.run();
	return (0);
}
