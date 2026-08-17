#include <iostream>
#include <mysql/mysql.h>

using namespace std;

int main()
{
    MYSQL* conn;

    // Initialize MySQL connection
    conn = mysql_init(NULL);

    if (conn == NULL)
    {
        cout << "MySQL initialization failed!" << endl;
        return 1;
    }

    // Connect to MySQL
    if (mysql_real_connect(
            conn,
            "localhost",
            "root",
            "harshit16sep@",
            "smart_inventory_db",
            3306,
            NULL,
            0
        ) == NULL)
    {
        cout << "Connection Failed!" << endl;
        cout << mysql_error(conn) << endl;

        mysql_close(conn);
        return 1;
    }

    cout << "Successfully Connected to MySQL Database!" << endl;

    mysql_close(conn);

    return 0;
}