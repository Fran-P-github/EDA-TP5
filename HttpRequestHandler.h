/**
 * @file HttpRequestHandler.h
 * @author Marc S. Ressl
 * @brief EDAoggle search engine
 * @version 0.4
 *
 * @copyright Copyright (c) 2022-2024 Marc S. Ressl
 */

#ifndef HTTPREQUESTHANDLER_H
#define HTTPREQUESTHANDLER_H

#include "HttpServer.h"

class HttpRequestHandler
{
public:
    HttpRequestHandler(std::string homePath);

    bool handleRequest(std::string url, HttpArguments arguments, std::vector<char> &response);

private:
    bool serve(std::string path, std::vector<char> &response);

    std::string homePath;
};

#endif
