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
		return (1);
	char *str = argv[1];
	std::cout << str << std::endl;
	// create Config
	std::cout << "Configuration loaded" << std::endl;

	Server	server;
	server.run();
	return (0);
}
