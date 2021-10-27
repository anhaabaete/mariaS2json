# mariaS2json
A C++ adventure to a fast return JSON from maria DB or Mysql.

## About

This project opensource is very simple.

A application to runs in CGI/FastCGI or interpreting SQLs query to return data from a MariaDB or MySQL in JSON format. Without programing a backend application. Only with a logic struture design in simples litle files.

### Why?

It is must more fast and scalable.

### To colaborate

Feel free to do this.
Contact me on telegram https://t.me/anhaabaete

## To compile
<code>
g++ main.c ../inc/ini.c -o ../mariaS2json -lmysqlclient
</code>

### Configuration

- Install apache or another server HTTP
- Enable CGI pr FastCGI mods
- Compile and save the compiled file at CGI/FastCGI path
- Write the service (SQL querys) in .sql files

#### About Me
My name Tiago Neves
Im living in São Paulo , Brazil.
