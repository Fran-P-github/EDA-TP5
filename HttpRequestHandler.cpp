/**
 * @file HttpRequestHandler.h
 * @author Marc S. Ressl
 * @brief EDAoggle search engine
 * @version 0.3
 *
 * @copyright Copyright (c) 2022-2024 Marc S. Ressl
 */

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sqlite3.h>     
#include <chrono>        
#include <algorithm>  
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>

#include "HttpRequestHandler.h"

using namespace std;

std::vector<std::string> splitBySpaces(const std::string& text) { //function to split string by spaces
    std::istringstream iss(text);
    std::vector<std::string> words;
    std::string word;

    while (iss >> word) {
        words.push_back(word);
    }

    return words;
}

HttpRequestHandler::HttpRequestHandler(string homePath)
{
    this->homePath = homePath;
}

/**
 * @brief Serves a webpage from file
 *
 * @param url The URL
 * @param response The HTTP response
 * @return true URL valid
 * @return false URL invalid
 */
bool HttpRequestHandler::serve(string url, vector<char> &response)
{
    // Blocks directory traversal
    // e.g. https://www.example.com/show_file.php?file=../../MyFile
    // * Builds absolute local path from url
    // * Checks if absolute local path is within home path
    auto homeAbsolutePath = filesystem::absolute(homePath);
    auto relativePath = homeAbsolutePath / url.substr(1);
    string path = filesystem::absolute(relativePath.make_preferred()).string();

    if (path.substr(0, homeAbsolutePath.string().size()) != homeAbsolutePath)
        return false;

    // Serves file
    ifstream file(path);
    if (file.fail())
        return false;

    file.seekg(0, ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, ios::beg);

    response.resize(fileSize);
    file.read(response.data(), fileSize);

    return true;
}

bool HttpRequestHandler::handleRequest(string url,
                                               HttpArguments arguments,
                                               vector<char> &response)
{
    string searchPage = "/search";
    if (url.substr(0, searchPage.size()) == searchPage)
    {
        string searchString;
        if (arguments.find("q") != arguments.end())
            searchString = arguments["q"];

        // Header
        string responseString = string("<!DOCTYPE html>\
<html>\
\
<head>\
    <meta charset=\"utf-8\" />\
    <title>EDAoogle</title>\
    <link rel=\"preload\" href=\"https://fonts.googleapis.com\" />\
    <link rel=\"preload\" href=\"https://fonts.gstatic.com\" crossorigin />\
    <link href=\"https://fonts.googleapis.com/css2?family=Inter:wght@400;800&display=swap\" rel=\"stylesheet\" />\
    <link rel=\"preload\" href=\"../css/style.css\" />\
    <link rel=\"stylesheet\" href=\"../css/style.css\" />\
</head>\
\
<body>\
    <article class=\"edaoogle\">\
        <div class=\"title\"><a href=\"/\">EDAoogle</a></div>\
        <div class=\"search\">\
            <form action=\"/search\" method=\"get\">\
                <input type=\"text\" name=\"q\" value=\"" +
                                       searchString + "\" autofocus>\
            </form>\
        </div>\
        ");

        // YOUR JOB: fill in results
        float searchTime = 0.1F;

        auto start = chrono::high_resolution_clock::now();

        sqlite3* db;
        sqlite3_open("index.db", &db);

        sqlite3_stmt* stmt;

		std::vector<std::string> words = splitBySpaces(searchString);
        unordered_map<string, int> score;

        for (auto& w : words) {
            int wordId = -1;
            string sql = "SELECT id FROM words WHERE word = '" + w + "';";
            if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL) != SQLITE_OK) {
                cerr << "SQL error (prepare): " << sqlite3_errmsg(db) << endl;
            }
            else if (sqlite3_step(stmt) == SQLITE_ROW)
                wordId = sqlite3_column_int(stmt, 0);
            sqlite3_finalize(stmt);

            //vector<pair<string, int>> docs;

            if (wordId != -1) {
                sql = "SELECT documents.url, word_occurrences.frequency "
                    "FROM word_occurrences "
                    "JOIN documents ON documents.id = word_occurrences.document_id "
                    "WHERE word_occurrences.word_id = " + to_string(wordId) + ";";

                sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    string url = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                    int freq = sqlite3_column_int(stmt, 1);
                    //docs.push_back({ url, freq });
                    score[url] += freq;
                }
                sqlite3_finalize(stmt);
            }
        }
        vector<pair<string, int>> docs(score.begin(), score.end());

        sort(docs.begin(), docs.end(), [](auto& a, auto& b) {
            return a.second > b.second;
            });

        sqlite3_close(db);

        vector<string> results;
        for (auto& d : docs)
            results.push_back(d.first);

        auto end = chrono::high_resolution_clock::now();
        searchTime = chrono::duration<float>(end - start).count();

        // Print search results
        responseString += "<div class=\"results\">" + to_string(results.size()) +
                          " results (" + to_string(searchTime) + " seconds):</div>";
        for (auto &result : results)
            responseString += "<div class=\"result\"><a href=\"" +
                              result + "\">" + result + "</a></div>";

        // Trailer
        responseString += "    </article>\
</body>\
</html>";

        response.assign(responseString.begin(), responseString.end());

        return true;
    }
    else
        return serve(url, response);

    return false;
}
