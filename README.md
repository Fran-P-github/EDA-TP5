# EDA-TP5

# Integrantes del grupo

- Rocco Diaz Parisi 
- Juan Ignacio Fogolin Lagares
- Francisco Paredes Alonso
- Santiago Resnik

# Funcionamiento del codigo

## mkindex.cpp:

- Procesamiento de Archivos

- Recorre todos los archivos .html dentro de www/wiki

- Lee el contenido del archivo

- Elimina etiquetas HTML

- Extrae palabras normalizadas (solo letras/números y en minúsculas)

- Creación de la Base de Datos

- Genera tres tablas:

- documents(id, url)
- Guarda cada archivo indexado

- words(id, word)
- Una entrada por cada palabra única

- word_occurrences(word_id, document_id, frequency)
- Guarda cuántas veces aparece cada palabra en cada documento

## Optimización

- Se usa una cache (unordered_map) para evitar buscar repetidamente IDs de palabras

- Se corre todo dentro de una transacción para acelerar la indexación

- Se crean índices SQL para optimizar búsquedas posteriores

## HttpRequestHandler.cpp:

### Proceso de Búsqueda

- Cuando el usuario escribe algo en la barra de búsqueda:

- Separación en Palabras
- El texto ingresado se divide en palabras usando espacios.

- Consulta de cada palabra
- Para cada palabra:

- Se busca su word_id en la tabla words

- Si existe, se obtienen todos los documentos donde aparece

- Se acumula la frecuencia en un mapa score[url] += freq

- Esto permite búsquedas multi-palabra sumando relevancias.

### Ranking de Resultados
- Se transforma el mapa en un vector y se ordena:

- Documentos con mayor frecuencia total primero

- Documentos que contienen más palabras de la query quedan arriba

- Generación de la Página HTML
- Se construye dinámicamente la sección de resultados con links a las páginas encontradas.

## Seguridad

- El sistema evita inyecciones SQL destructivas porque:

- Solo se ejecutan consultas SELECT

- Cada palabra se consulta individualmente

- SQLite no permite múltiples sentencias por consulta preparada

- Errores de sintaxis solo afectan esa palabra, no el sistema