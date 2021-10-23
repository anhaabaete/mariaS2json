
#include <stdlib.h>
#include <iostream>

#include "mysql_driver.h"
#include "mysql_connection.h"

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <fstream>

using namespace std;
using namespace sql::mysql;

/**
 * getServiceFromFile
 * read the file of service request and return the SQL query to set by ENV variables 
 **/
string getServiceFromFile(string service_path) {
  fstream my_file;
  string sql_query;
  my_file.open(service_path,ios::in);
  if (!my_file) {
		//TODO return HTTP error 5xx
	}
	else {
		string line;

		while (getline(my_file,line)) {
        sql_query += line;
		}

	}
	my_file.close();
  return sql_query;
}

int main(void)
{

cout << "Content-type:application/json\r\n\r\n" << endl;

try {
  sql::Driver *driver;
  sql::Connection *con;
  sql::Statement *stmt;
  sql::ResultSet *res;

  /* Create a connection */
  driver = get_mysql_driver_instance();
  con = driver->connect("tcp://127.0.0.1:3306", "phpmyadmin", "supersecretpassword");
  /* Connect to the MySQL test database */
  con->setSchema("superset");

  stmt = con->createStatement();
  res = stmt->executeQuery(getServiceFromFile("../test.sql"));
  
  string comma;

  cout << "{\"error\":0,\"data\":[";

  while (res->next()) {
    if (!res->next()) { comma = ""; }
    /* Access column data by alias or column name */
    cout << res->getString("_message") << comma;
    comma = ",";
  }
  cout << "]}";
  delete res;
  delete stmt;
  delete con;

} catch (sql::SQLException &e) {
  cout << "{\"error\":1,\"data\":[\"";
  cout << "# ERR: SQLException in " << __FILE__ << "(" << __FUNCTION__ << ") on line " << __LINE__ << " ";
  cout << "# ERR: " << e.what();
  cout << " (MySQL error code: " << e.getErrorCode();
  cout << ", SQLState: " << e.getSQLState() << " )";
  cout << "\"]}";
}

cout << endl;

return EXIT_SUCCESS;
}