/**
 * @file mkindex.cpp
 * @author Marc S. Ressl
 * @brief Makes a database index
 * @version 0.3
 *
 * @copyright Copyright (c) 2022-2024 Marc S. Ressl
 */

#include <iostream>
#include <string>
#include <sqlite3.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include "CommandLineParser.h"
#include <unordered_map>

using namespace std;
namespace fs = std::filesystem;

static unordered_map<string, int> wordIdCache;

static int onDatabaseEntry(void* userdata, int argc, char** argv, char** azColName);
static string readFile(const string& filepath);
static string removeHTMLTags(const string& html);
static vector<string> extractWords(const string& text);
static void indexDocument(sqlite3* database, const string& documentUrl, const vector<string>& words);

/**
 * @brief Inserta un documento y sus palabras asociadas en la base de datos.
 *
 * Inserta el documento (si no existe), obtiene su ID, calcula la frecuencia
 * de cada palabra y las inserta en las tablas `words` y `word_occurrences`.
 *
 * @param database Puntero a la base de datos SQLite.
 * @param documentUrl Ruta URL del documento (ej: "/wiki/algo.html").
 * @param words Vector con todas las palabras extraídas del documento.
 */
void indexDocument(sqlite3* database, const string& documentUrl, const vector<string>& words) {
    string sql = "INSERT OR IGNORE INTO documents(url) VALUES('" + documentUrl + "');";
    char* databaseErrorMessage;
    int documentId = -1;
    if (sqlite3_exec(database, sql.c_str(), NULL, 0, &databaseErrorMessage) != SQLITE_OK) {
        cout << "Error: " << sqlite3_errmsg(database) << endl;
    }
    sql = "SELECT id FROM documents WHERE url='" + documentUrl + "';";
    if (sqlite3_exec(database, sql.c_str(), onDatabaseEntry, &documentId, &databaseErrorMessage) != SQLITE_OK) {
        cout << "Error: " << sqlite3_errmsg(database) << endl;
    }
    map<string, int> wordFrequency;
    for (const auto& word : words) {
        wordFrequency[word]++;
    }
    int wordId;
    for (const auto& [word, frequency] : wordFrequency) {
        auto it = wordIdCache.find(word);
        if (it == wordIdCache.end()) {
            wordId = -1;
            sql = "INSERT OR IGNORE INTO words (word) VALUES ('" + word + "');";
            if (sqlite3_exec(database, sql.c_str(), NULL, 0, &databaseErrorMessage) != SQLITE_OK) {
                cout << "Error: " << sqlite3_errmsg(database) << endl;
            }
            sql = "SELECT id FROM words WHERE word ='" + word + "';";
            if (sqlite3_exec(database, sql.c_str(), onDatabaseEntry, &wordId, &databaseErrorMessage) != SQLITE_OK) {
                cout << "Error: " << sqlite3_errmsg(database) << endl;
            }
            wordIdCache[word] = wordId;
        }
        else {
            wordId = it->second;
        }
        sql = "INSERT OR REPLACE INTO word_occurrences (word_id, document_id, frequency)"
            "VALUES ('" + to_string(wordId) + "','" + to_string(documentId) + "','" + to_string(frequency) + "');";
        if (sqlite3_exec(database, sql.c_str(), NULL, 0, &databaseErrorMessage) != SQLITE_OK) {
            cout << "Error: " << sqlite3_errmsg(database) << endl;
        }
    }
}

/**
 * @brief Lee el contenido completo de un archivo.
 *
 * @param filepath Ruta del archivo a leer.
 * @return string Contenido completo del archivo en texto.
 */
static string readFile(const string& filepath) {
    ifstream file(filepath);
    if (!file.is_open()) {
        cerr << "Error: No se pudo abrir " << filepath << endl;
        return "";
    }

    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

/**
 * @brief Elimina etiquetas HTML del texto manteniendo solo contenido visible.
 *
 * Recorre el texto carácter por carácter ignorando todo lo que esté entre '<' y '>'.
 *
 * @param html Texto original con etiquetas HTML.
 * @return string Texto plano sin etiquetas HTML.
 */
static string removeHTMLTags(const string& html) {
    string text;
    bool insideTag = false;
    for (char c : html) {
        if (c == '<') insideTag = true;
        else if (c == '>') insideTag = false;
        else if (!insideTag) text += c;
    }
    return text;
}

/**
 * @brief Extrae todas las palabras alfanuméricas de un texto.
 *
 * Convierte todas las letras a minúsculas. Las palabras consisten solo en caracteres alfanuméricos.
 *
 * @param text Texto del cual extraer palabras.
 * @return vector<string> Lista de palabras en minúsculas.
 */
static vector<string> extractWords(const string& text) {
    vector<string> words;
    string word;
    for (unsigned char c : text) {
        if (isalnum(c)) {
            word += tolower(c);
        }
        else if (!word.empty()) {
            words.push_back(word);
            word.clear();
        }
    }
    if (!word.empty()) words.push_back(word);
    return words;
}

/**
 * @brief Callback para recuperar un valor entero de una consulta SQLite.
 *
 * Se usa para obtener el ID de una fila recién insertada o buscada.
 *
 * @param userdata Puntero que recibe el entero convertido.
 * @param argc Cantidad de columnas.
 * @param argv Valores de las columnas.
 * @param azColName Nombres de las columnas.
 * @return int Siempre retorna 0 para indicar éxito.
 */
static int onDatabaseEntry(void* userdata,
    int argc,
    char** argv,
    char** azColName)
{
    *((int*)userdata) = stoi(argv[0]);
    return 0;
}

/**
 * @brief Punto de entrada principal del programa.
 *
 * Procesa archivos HTML en `wwwPath/wiki`, extrae su contenido,
 * lo tokeniza en palabras y crea un índice persistente en SQLite.
 *
 * @param argc Cantidad de argumentos.
 * @param argv Lista de argumentos.
 * @return int Código de salida del programa.
 */
int main(int argc,
    const char* argv[])
{
    wordIdCache.clear();
    CommandLineParser parser(argc, argv);

    // Configuration
    int port = 8000;
    string wwwPath;

    // Parse command line
    if (!parser.hasOption("-h"))
    {
        cout << "error: WWW_PATH must be specified." << endl;

        return 1;
    }

    wwwPath = parser.getOption("-h");

    char* databaseFile = "index.db";
    sqlite3* database;
    char* databaseErrorMessage;

    // Open database file
    cout << "Opening database..." << endl;
    if (sqlite3_open(databaseFile, &database) != SQLITE_OK)
    {
        cout << "Can't open database: " << sqlite3_errmsg(database) << endl;

        return 1;
    }

    // Create a sample table
    cout << "Creating table..." << endl;
    if (sqlite3_exec(database,
        "CREATE TABLE IF NOT EXISTS documents("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "url TEXT UNIQUE NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS words("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "word TEXT UNIQUE NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS word_occurrences("
        "word_id INTEGER,"
        "document_id INTEGER,"
        "frequency INTEGER,"
        "FOREIGN KEY(word_id) REFERENCES words(id),"
        "FOREIGN KEY(document_id) REFERENCES documents(id)"
        ");",
        NULL,
        0,
        &databaseErrorMessage) != SQLITE_OK)
        cout << "Error: " << sqlite3_errmsg(database) << endl;

    // Delete previous entries if table already existed
    cout << "Deleting previous entries..." << endl;
    if (sqlite3_exec(database,
        "DELETE FROM documents;",
        NULL,
        0,
        &databaseErrorMessage) != SQLITE_OK)
        cout << "Error: " << sqlite3_errmsg(database) << endl;

    if (sqlite3_exec(database,
        "DELETE FROM words;",
        NULL,
        0,
        &databaseErrorMessage) != SQLITE_OK)
        cout << "Error: " << sqlite3_errmsg(database) << endl;

    if (sqlite3_exec(database,
        "DELETE FROM word_occurrences;",
        NULL,
        0,
        &databaseErrorMessage) != SQLITE_OK)
        cout << "Error: " << sqlite3_errmsg(database) << endl;


    cout << "Beginning transaction..." << endl;
    if (sqlite3_exec(database, "BEGIN TRANSACTION;", NULL, 0, &databaseErrorMessage) != SQLITE_OK)
        cout << "Error: " << sqlite3_errmsg(database) << endl;

    int fileCount = 0;
    for (const auto& entry : fs::directory_iterator(wwwPath + "/wiki")) {
        if (entry.path().extension() == ".html") {
            string filepath = entry.path().string();
            string filename = entry.path().filename().string();

            cout << "Procesando: " << filename << endl;

            // 1. Leer el archivo
            string htmlContent = readFile(filepath);
            if (htmlContent.empty()) continue;

            // 2. Eliminar etiquetas HTML
            string textContent = removeHTMLTags(htmlContent);

            // 3. Extraer palabras
            vector<string> words = extractWords(textContent);

            // 4. Guardar en la base de datos
            string documentUrl = "/wiki/" + filename;

            cout << "Indexando: " << filename << endl;
            indexDocument(database, documentUrl, words);

            fileCount++;
        }
    }

    cout << "Committing transaction..." << endl;
    if (sqlite3_exec(database, "COMMIT;", NULL, 0, &databaseErrorMessage) != SQLITE_OK)
        cout << "Error: " << sqlite3_errmsg(database) << endl;

    cout << "Creating indexes..." << endl;
    if (sqlite3_exec(database,
        "CREATE INDEX IF NOT EXISTS idx_word ON words(word);"
        "CREATE INDEX IF NOT EXISTS idx_word_occurrences "
        "ON word_occurrences(word_id, document_id);",
        NULL,
        0,
        &databaseErrorMessage) != SQLITE_OK)
        cout << "Error: " << sqlite3_errmsg(database) << endl;

    // Close database
    cout << "Closing database..." << endl;
    sqlite3_close(database);
}
