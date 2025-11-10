/**
 * @file HttpServer.h
 * @author Marc S. Ressl
 * @brief Simple interface to libmicrohttpd
 * @version 0.3
 *
 * @copyright Copyright (c) 2022-2024 Marc S. Ressl
 */

#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <microhttpd.h>

#include <map>
#include <string>
#include <vector>

typedef std::map<std::string, std::string> HttpArguments;

class HttpRequestHandler;

class HttpServer
{
public:
    HttpServer(int port);
    ~HttpServer();

    bool isRunning();
    void setHttpRequestHandler(HttpRequestHandler *httpRequestHandler);

private:
    MHD_Daemon *daemon;
    HttpRequestHandler *httpRequestHandler;

    // Grants private access to libmicrohttp callback
    friend MHD_Result httpRequestHandlerCallback(void *cls, struct MHD_Connection *connection,
                                                 const char *url, const char *method, const char *version,
                                                 const char *upload_data, size_t *upload_data_size,
                                                 void **con_cls);
};

#endif
