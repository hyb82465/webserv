/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zhma <zhma@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/04 16:44:33 by yihe              #+#    #+#             */
/*   Updated: 2026/08/15 12:16:46 by zhma             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "ConfigParser.hpp"
#include "ServerConfig.hpp"
#include "TokenStream.hpp"
#include "Tokenizer.hpp"

#include <iostream>

static void printConfig(
    const std::vector<ServerConfig> &servers)
{
    std::cout << std::endl;
    std::cout << "========== Configuration =========="
              << std::endl;

    std::cout << "Number of servers: "
              << servers.size()
              << std::endl;

    for (size_t i = 0; i < servers.size(); ++i)
    {
        const ServerConfig &server = servers[i];

        std::cout << std::endl;
        std::cout << "---------- Server "
                  << i + 1
                  << " ----------"
                  << std::endl;

        /*
         * listen
         */
        std::cout << "Listen:"
                  << std::endl;

        for (size_t j = 0;
             j < server.listens.size();
             ++j)
        {
            const ListenConfig &listen =
                server.listens[j];

            std::cout << "  "
                      << listen.host
                      << ":"
                      << listen.port
                      << std::endl;
        }

        /*
         * server root
         */
        std::cout << "Root: "
                  << server.root
                  << std::endl;

        /*
         * server index
         */
        std::cout << "Index: "
                  << server.index
                  << std::endl;

        /*
         * client max body size
         */
        std::cout << "Client max body size: "
                  << server.client_max_body_size
                  << std::endl;

        /*
         * error pages
         */
        std::cout << "Error pages:"
                  << std::endl;

        std::map<int, std::string>::const_iterator errorIt;

        for (errorIt = server.error_pages.begin();
             errorIt != server.error_pages.end();
             ++errorIt)
        {
            std::cout << "  "
                      << errorIt->first
                      << " -> "
                      << errorIt->second
                      << std::endl;
        }

        /*
         * locations
         */
        std::cout << "Locations: "
                  << server.locations.size()
                  << std::endl;

        for (size_t j = 0;
             j < server.locations.size();
             ++j)
        {
            const LocationConfig &location =
                server.locations[j];

            std::cout << std::endl;

            std::cout << "  Location: "
                      << location.path
                      << std::endl;

            /*
             * location root
             */
            std::cout << "    Root: "
                      << location.root
                      << std::endl;

            /*
             * location index
             */
            std::cout << "    Index: "
                      << location.index
                      << std::endl;

            /*
             * methods
             */
            std::cout << "    Methods:";

            for (size_t k = 0;
                 k < location.methods.size();
                 ++k)
            {
                std::cout << " "
                          << location.methods[k];
            }

            std::cout << std::endl;

            /*
             * autoindex
             */
            std::cout << "    Autoindex: "
                      << (location.autoindex
                          ? "on"
                          : "off")
                      << std::endl;

            /*
             * upload
             */
            std::cout << "    Upload store: "
                      << location.upload_store
                      << std::endl;

            /*
             * redirect
             */
            if (location.redirectCode != 0)
            {
                std::cout
                    << "    Redirect: "
                    << location.redirectCode
                    << " -> "
                    << location.redirectUrl
                    << std::endl;
            }
            else
            {
                std::cout
                    << "    Redirect: none"
                    << std::endl;
            }

            /*
             * CGI
             */
            std::cout << "    CGI:"
                      << std::endl;

            std::map<std::string, std::string>::const_iterator cgiIt;

            for (cgiIt = location.cgi.begin();
                 cgiIt != location.cgi.end();
                 ++cgiIt)
            {
                std::cout
                    << "      "
                    << cgiIt->first
                    << " -> "
                    << cgiIt->second
                    << std::endl;
            }

            if (location.cgi.empty())
            {
                std::cout
                    << "      none"
                    << std::endl;
            }
        }
    }

    std::cout << std::endl;
    std::cout << "=================================="
              << std::endl;
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		std::cerr << "Usage: ./webserv <configuration_file>"
				  << std::endl;
		return (1);
	}

	// // create Config
	
	try
	{
		ConfigParser parser;
		std::vector<ServerConfig> servers;
		servers = parser.parse(argv[1]);
		//printf("debug 1\n");
		// for(size_t i = 0; i<servers.size();++i)
		// {
		// 	std::cout <<"server "<< i + 1 << std::endl;
		// 	for (size_t j = 0; j < servers[i].listens.size();++j)
		// 	{
		// 		std::cout << "Listen: " << servers[i].listens[j].host << ": " << servers[i].listens[j].port
		// 					<<std::endl;
		// 	}
			
		// 	std::cout << "Root: " << servers[0].root << std::endl;
		// 	std::cout << "Index: " << servers[0].index << std::endl;
		// }
			
		 std::cout << "Configuration file parsed successfully" << std::endl;
		printConfig(servers);
		
		
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