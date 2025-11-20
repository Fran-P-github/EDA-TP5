    /**
 * @file edahttpd.cpp
 * @author Marc S. Ressl
 * @brief Manages the edahttpd server
 * @version 0.3
 *
 * @copyright Copyright (c) 2022-2024 Marc S. Ressl
 */

#include <iostream>

#include <microhttpd.h>

#include "CommandLineParser.h"
#include "HttpServer.h"
#include "HttpRequestHandler.h"

using namespace std;

void printHelp()
{
    cout << "Usage: edahttpd -h WWW_PATH [-p PORT] " << endl;
};

int main(int argc, const char *argv[])
{
    CommandLineParser parser(argc, argv);

    // Configuration
    int port = 8000;
    string wwwPath;

    // Parse command line
    if (!parser.hasOption("-h"))
    {
        cout << "error: WWW_PATH must be specified." << endl;

        printHelp();

        return 1;
    }
   
    wwwPath = parser.getOption("-h");

    if (parser.hasOption("-p"))
        port = stoi(parser.getOption("-p"));

    // Start server
    HttpServer server(port);

    HttpRequestHandler edaOogleHttpRequestHandler(wwwPath);
    server.setHttpRequestHandler(&edaOogleHttpRequestHandler);

    if (server.isRunning())
    {
        cout << "Running server..." << endl;

        // Wait for keyboard entry
        char value;
        cin >> value;

        cout << "Stopping server..." << endl;
    }
}
