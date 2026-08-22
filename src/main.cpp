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
#include "ServerConfig.hpp"
#include "TokenStream.hpp"
#include "Tokenizer.hpp"

#include <iostream>

// static void printConfig(
//     const std::vector<ServerConfig> &servers)
// {
//     std::cout << std::endl;
//     std::cout << "========== Configuration =========="
//               << std::endl;

//     std::cout << "Number of servers: "
//               << servers.size()
//               << std::endl;

//     for (size_t i = 0; i < servers.size(); ++i)
//     {
//         const ServerConfig &server = servers[i];

//         std::cout << std::endl;
//         std::cout << "---------- Server "
//                   << i + 1
//                   << " ----------"
//                   << std::endl;

//         /*
//          * listen
//          */
//         std::cout << "Listen:"
//                   << std::endl;

//         const std::vector<ListenConfig> &listens =
//             server.getListens();

//         for (size_t j = 0;
//             j < listens.size();
//             ++j)
//         {
//             const ListenConfig &listen = listens[j];

//             std::cout << "  "
//                     << listen.getHost()
//                     << ":"
//                     << listen.getPort()
//                     << std::endl;
//         }

//         /*
//          * server root
//          */
//         std::cout << "Root: "
//                   << server.getRoot()
//                   << std::endl;

//         /*
//          * server index
//          */
//         std::cout << "Index: "
//                   << server.getIndex()
//                   << std::endl;

//         /*
//          * client max body size
//          */
//         std::cout << "Client max body size: "
//                   << server.getClientMaxBodySize()
//                   << std::endl;

//         /*
//          * error pages
//          */
//         std::cout << "Error pages:"
//                   << std::endl;

//         std::map<int, std::string>::const_iterator errorIt;
        
//         const std::map<int, std::string> &errorPages =
//             server.getErrorPages();

//         for (errorIt = errorPages.begin();
//             errorIt != errorPages.end();
//             ++errorIt)
//         {
//             std::cout << "  "
//                       << errorIt->first
//                       << " -> "
//                       << errorIt->second
//                       << std::endl;
//         }

//         /*
//          * locations
//          */
//         const std::vector<LocationConfig> &locations =
//             server.getLocations();
//         std::cout << "Locations: "
//                   << locations.size()
//                   << std::endl;

//         for (size_t j = 0;
//              j < locations.size();
//              ++j)
//         {
//             const LocationConfig &location =
//                 locations[j];

//             std::cout << std::endl;

//             std::cout << "  Location: "
//                       << location.getPath()
//                       << std::endl;

//             /*
//              * location root
//              */
//             std::cout << "    Root: "
//                       << location.getRoot()
//                       << std::endl;

//             /*
//              * location index
//              */
//             std::cout << "    Index: "
//                       << location.getIndex()
//                       << std::endl;

//             /*
//              * methods
//              */
//             std::cout << "    Methods:";
//             const std::vector<std::string> &methods = location.getMethods();
//             for (size_t k = 0;
//                  k < methods.size();
//                  ++k)
//             {
//                 std::cout << " "
//                           << methods[k];
//             }

//             std::cout << std::endl;

//             /*
//              * autoindex
//              */
//             std::cout << "    Autoindex: "
//                       << (location.getAutoindex()
//                           ? "on"
//                           : "off")
//                       << std::endl;

//             /*
//              * upload
//              */
//             std::cout << "    Upload store: "
//                       << location.getUploadStore()
//                       << std::endl;

//             /*
//              * redirect
//              */
//             std::cout << "    Redirect code: "
//                       << location.getRedirectCode()
//                       << std::endl;
//             std::cout << "    Redirect URL: "
//                       << location.getRedirectUrl()
//                       << std::endl;
            
//             /*
//             * CGI
//             */
//            std::cout << "    CGI:"
//            << std::endl;
           
//            const std::map<std::string, std::string> &cgi = location.getCgi();
//            std::map<std::string, std::string>::const_iterator it;
//            for(it = cgi.begin();it != cgi.end();++it)
//            {
//                std::cout << it->first << "-> " << it->second << std::endl; 
//            }


//             if (location.getCgi().empty())
//             {
//                 std::cout
//                     << "      none"
//                     << std::endl;
//             }
//         }
//     }

//     std::cout << std::endl;
//     std::cout << "=================================="
//               << std::endl;
// }

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
			
		// std::cout << "Configuration file parsed successfully" << std::endl;
		// printConfig(servers);

		Server	server(servers);
		server.run();		
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return (1);
	}
	
	std::vector<std::string> tokens;

	return (0);
}