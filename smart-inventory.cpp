#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <stack>
#include <queue>
#include <algorithm>
#include <mysql/mysql.h>
using namespace std;

MYSQL *conn;
MYSQL *connectDatabase()
{
    MYSQL *conn = mysql_init(NULL);

    if (conn == NULL)
    {
        cout << "MySQL initialization failed!" << endl;
        return NULL;
    }

    if (mysql_real_connect(
            conn,
            "localhost",
            "root",
            "harshit16sep@",
            "smart_inventory_db",
            3306,
            NULL,
            0) == NULL)
    {
        cout << "Database Connection Failed!" << endl;
        cout << mysql_error(conn) << endl;

        mysql_close(conn);
        return NULL;
    }

    return conn;
}

class Product
{
public:
    int id;
    string name;
    string category;
    double price;
    int quantity;
};
class Order
{
public:
    int orderId;
    string customerName;
    int productId;
    int quantity;
};

vector<Product> products;
unordered_map<int, int> productIndex;
stack<Product> deletedProducts;
queue<Order> orders;

void addOrder()
{
    Order order;

    cout << "\nEnter Order ID: ";
    cin >> order.orderId;

    cin.ignore();

    cout << "Enter Customer Name: ";
    getline(cin, order.customerName);

    cout << "Enter Product ID: ";
    cin >> order.productId;

    cout << "Enter Quantity: ";
    cin >> order.quantity;

    if (order.quantity <= 0)
    {
        cout << "\nInvalid Quantity!\n";
        return;
    }

    // Check if product exists in MySQL
    string query =
        "SELECT product_id FROM products WHERE product_id = " +
        to_string(order.productId);

    if (mysql_query(conn, query.c_str()))
    {
        cout << "\nFailed to Check Product!\n";
        cout << mysql_error(conn) << endl;
        return;
    }

    MYSQL_RES *result = mysql_store_result(conn);

    if (result == NULL)
    {
        cout << "\nFailed to Get Product Data!\n";
        return;
    }

    MYSQL_ROW row = mysql_fetch_row(result);

    if (row == NULL)
    {
        cout << "\nProduct Not Found!\n";
        mysql_free_result(result);
        return;
    }

    mysql_free_result(result);

    // Add order to queue
    orders.push(order);

    cout << "\nOrder Added Successfully!\n";
}
void processOrder()
{
    if (orders.empty())
    {
        cout << "\nNo Orders Available!\n";
        return;
    }

    // Get the first order
    Order currentOrder = orders.front();

    int productId = currentOrder.productId;
    int orderQuantity = currentOrder.quantity;

    // Get product details from database
    string selectQuery =
        "SELECT name, price, quantity FROM products "
        "WHERE product_id = " +
        to_string(productId);

    if (mysql_query(conn, selectQuery.c_str()))
    {
        cout << "\nFailed to Find Product!\n";
        cout << mysql_error(conn) << endl;
        return;
    }

    MYSQL_RES *result = mysql_store_result(conn);

    if (result == NULL)
    {
        cout << "\nFailed to Get Product Data!\n";
        return;
    }

    MYSQL_ROW row = mysql_fetch_row(result);

    if (row == NULL)
    {
        cout << "\nProduct No Longer Exists!\n";

        mysql_free_result(result);

        // Remove invalid order
        orders.pop();

        return;
    }

    string productName = row[0];
    double price = stod(row[1]);
    int availableQuantity = stoi(row[2]);

    mysql_free_result(result);

    // Check stock
    if (orderQuantity > availableQuantity)
    {
        cout << "\nNot Enough Stock Available!\n";
        cout << "Available Quantity: "
             << availableQuantity << endl;

        cout << "Order remains in the queue.\n";

        return;
    }

    double totalAmount = price * orderQuantity;

    // Start transaction
    mysql_query(conn, "START TRANSACTION");

    // Reduce stock
    string updateQuery =
        "UPDATE products SET quantity = quantity - " +
        to_string(orderQuantity) +
        " WHERE product_id = " +
        to_string(productId);

    if (mysql_query(conn, updateQuery.c_str()))
    {
        cout << "\nFailed to Update Stock!\n";
        cout << mysql_error(conn) << endl;

        mysql_query(conn, "ROLLBACK");
        return;
    }

    // Add sale record
    string saleQuery =
        "INSERT INTO sales "
        "(product_id, quantity_sold, total_amount) VALUES (" +
        to_string(productId) + ", " +
        to_string(orderQuantity) + ", " +
        to_string(totalAmount) + ")";

    if (mysql_query(conn, saleQuery.c_str()))
    {
        cout << "\nFailed to Save Sale!\n";
        cout << mysql_error(conn) << endl;

        mysql_query(conn, "ROLLBACK");
        return;
    }

    // Complete transaction
    if (mysql_query(conn, "COMMIT"))
    {
        cout << "\nFailed to Process Order!\n";
        cout << mysql_error(conn) << endl;

        mysql_query(conn, "ROLLBACK");
        return;
    }

    // Remove successfully processed order
    orders.pop();

    cout << "\n========== ORDER PROCESSED ==========\n";

    cout << "Order ID      : "
         << currentOrder.orderId << endl;

    cout << "Customer Name : "
         << currentOrder.customerName << endl;

    cout << "Product       : "
         << productName << endl;

    cout << "Quantity      : "
         << orderQuantity << endl;

    cout << "Total Amount  : "
         << totalAmount << endl;

    cout << "\nOrder Processed Successfully!\n";
}
void updateProductIndex()
{
    productIndex.clear();

    for (int i = 0; i < products.size(); i++)
    {
        productIndex[products[i].id] = i;
    }
}
void sortProducts()
{
    if (products.empty())
    {
        cout << "\nNo Products Available!\n";
        return;
    }

    int choice;

    cout << "\n========== SORT PRODUCTS ==========\n";
    cout << "1. Sort by Name\n";
    cout << "2. Sort by Price\n";
    cout << "3. Sort by Quantity\n";

    cout << "Enter your choice: ";
    cin >> choice;

    if (choice == 1)
    {
        sort(products.begin(), products.end(),
             [](Product a, Product b)
             {
                 return a.name < b.name;
             });

        cout << "\nProducts Sorted by Name!\n";
    }

    else if (choice == 2)
    {
        sort(products.begin(), products.end(),
             [](Product a, Product b)
             {
                 return a.price < b.price;
             });

        cout << "\nProducts Sorted by Price!\n";
    }

    else if (choice == 3)
    {
        sort(products.begin(), products.end(),
             [](Product a, Product b)
             {
                 return a.quantity < b.quantity;
             });

        cout << "\nProducts Sorted by Quantity!\n";
    }

    else
    {
        cout << "\nInvalid Choice!\n";
        return;
    }

    // Vector indexes changed after sorting
    updateProductIndex();
}

void addProduct()
{
    Product p;

    cout << "\nEnter Product ID: ";
    cin >> p.id;

    cin.ignore();

    cout << "Enter Product Name: ";
    getline(cin, p.name);

    cout << "Enter Product Category: ";
    getline(cin, p.category);

    cout << "Enter Product Price: ";
    cin >> p.price;

    cout << "Enter Product Quantity: ";
    cin >> p.quantity;

    string query =
        "INSERT INTO products (product_id, name, category, price, quantity) VALUES (" +
        to_string(p.id) + ", '" +
        p.name + "', '" +
        p.category + "', " +
        to_string(p.price) + ", " +
        to_string(p.quantity) + ")";

    if (mysql_query(conn, query.c_str()))
    {
        cout << "\nFailed to Add Product!\n";
        cout << mysql_error(conn) << endl;
        return;
    }

    // Also add to vector
    products.push_back(p);

    // Update unordered_map
    productIndex[p.id] = products.size() - 1;

    cout << "\nProduct Added Successfully and Saved to Database!\n";
}
void displayProducts()
{
    string query = "SELECT * FROM products";

    if (mysql_query(conn, query.c_str()))
    {
        cout << "\nFailed to Fetch Products!\n";
        cout << mysql_error(conn) << endl;
        return;
    }

    MYSQL_RES *result = mysql_store_result(conn);

    if (result == NULL)
    {
        cout << "\nNo Product Data Found!\n";
        return;
    }

    MYSQL_ROW row;

    cout << "\n==============================================\n";
    cout << "             ALL PRODUCTS\n";
    cout << "==============================================\n";

    cout << "ID\tName\t\tCategory\tPrice\tQuantity\n";
    cout << "----------------------------------------------\n";

    while ((row = mysql_fetch_row(result)))
    {
        cout << row[0] << "\t"
             << row[1] << "\t\t"
             << row[2] << "\t"
             << row[3] << "\t"
             << row[4] << endl;
    }

    mysql_free_result(result);
}
void searchProduct()
{
    int id;

    cout << "\nEnter Product ID to Search: ";
    cin >> id;

    string query =
        "SELECT * FROM products WHERE product_id = " +
        to_string(id);

    // Execute query
    if (mysql_query(conn, query.c_str()))
    {
        cout << "\nSearch Failed!\n";
        cout << mysql_error(conn) << endl;
        return;
    }

    // Store result
    MYSQL_RES *result = mysql_store_result(conn);

    if (result == NULL)
    {
        cout << "\nFailed to Get Product Data!\n";
        return;
    }

    MYSQL_ROW row = mysql_fetch_row(result);

    // Check if product exists
    if (row == NULL)
    {
        cout << "\nProduct Not Found!\n";
        mysql_free_result(result);
        return;
    }

    cout << "\n========== PRODUCT FOUND ==========\n";

    cout << "Product ID : " << row[0] << endl;
    cout << "Name       : " << row[1] << endl;
    cout << "Category   : " << row[2] << endl;
    cout << "Price      : " << row[3] << endl;
    cout << "Quantity   : " << row[4] << endl;

    mysql_free_result(result);
}
void updateProduct()
{
    int id;
    string name;
    string category;
    double price;
    int quantity;

    cout << "\nEnter Product ID to Update: ";
    cin >> id;

    cin.ignore();

    cout << "Enter New Product Name: ";
    getline(cin, name);

    cout << "Enter New Category: ";
    getline(cin, category);

    cout << "Enter New Price: ";
    cin >> price;

    cout << "Enter New Quantity: ";
    cin >> quantity;

    string query =
        "UPDATE products SET "
        "name = '" +
        name +
        "', category = '" + category +
        "', price = " + to_string(price) +
        ", quantity = " + to_string(quantity) +
        " WHERE product_id = " + to_string(id);

    if (mysql_query(conn, query.c_str()))
    {
        cout << "\nFailed to Update Product!\n";
        cout << mysql_error(conn) << endl;
        return;
    }

    if (mysql_affected_rows(conn) == 0)
    {
        cout << "\nProduct Not Found!\n";
        return;
    }

    cout << "\nProduct Updated Successfully!\n";
}
void deleteProduct()
{
    int id;

    cout << "\nEnter Product ID to Delete: ";
    cin >> id;

    // First, get the product details
    string selectQuery =
        "SELECT * FROM products WHERE product_id = " +
        to_string(id);

    if (mysql_query(conn, selectQuery.c_str()))
    {
        cout << "\nFailed to Find Product!\n";
        cout << mysql_error(conn) << endl;
        return;
    }

    MYSQL_RES *result = mysql_store_result(conn);

    if (result == NULL)
    {
        cout << "\nFailed to Get Product Data!\n";
        return;
    }

    MYSQL_ROW row = mysql_fetch_row(result);

    if (row == NULL)
    {
        cout << "\nProduct Not Found!\n";
        mysql_free_result(result);
        return;
    }

    // Save product before deleting
    Product deletedProduct;

    deletedProduct.id = stoi(row[0]);
    deletedProduct.name = row[1];
    deletedProduct.category = row[2];
    deletedProduct.price = stod(row[3]);
    deletedProduct.quantity = stoi(row[4]);

    mysql_free_result(result);

    // Delete from database
    string deleteQuery =
        "DELETE FROM products WHERE product_id = " +
        to_string(id);

    if (mysql_query(conn, deleteQuery.c_str()))
    {
        cout << "\nFailed to Delete Product!\n";
        cout << mysql_error(conn) << endl;
        return;
    }

    // Save deleted product in stack
    deletedProducts.push(deletedProduct);

    cout << "\nProduct Deleted Successfully!\n";
}
void undoDelete()
{
    if (deletedProducts.empty())
    {
        cout << "\nNothing to Undo!\n";
        return;
    }

    Product restoredProduct = deletedProducts.top();

    string query =
        "INSERT INTO products "
        "(product_id, name, category, price, quantity) VALUES (" +
        to_string(restoredProduct.id) + ", '" +
        restoredProduct.name + "', '" +
        restoredProduct.category + "', " +
        to_string(restoredProduct.price) + ", " +
        to_string(restoredProduct.quantity) + ")";

    if (mysql_query(conn, query.c_str()))
    {
        cout << "\nFailed to Restore Product!\n";
        cout << mysql_error(conn) << endl;
        return;
    }

    deletedProducts.pop();

    cout << "\nProduct Restored Successfully!\n";
}
void sellProduct()
{
    int id;
    int quantity;

    cout << "\nEnter Product ID: ";
    cin >> id;

    cout << "Enter Quantity to Sell: ";
    cin >> quantity;

    if (quantity <= 0)
    {
        cout << "\nInvalid Quantity!\n";
        return;
    }

    // Find product in database
    string selectQuery =
        "SELECT price, quantity FROM products WHERE product_id = " +
        to_string(id);

    if (mysql_query(conn, selectQuery.c_str()))
    {
        cout << "\nFailed to Find Product!\n";
        cout << mysql_error(conn) << endl;
        return;
    }

    MYSQL_RES *result = mysql_store_result(conn);

    if (result == NULL)
    {
        cout << "\nFailed to Get Product Data!\n";
        return;
    }

    MYSQL_ROW row = mysql_fetch_row(result);

    if (row == NULL)
    {
        cout << "\nProduct Not Found!\n";
        mysql_free_result(result);
        return;
    }

    double price = stod(row[0]);
    int availableQuantity = stoi(row[1]);

    mysql_free_result(result);

    // Check stock
    if (quantity > availableQuantity)
    {
        cout << "\nNot Enough Stock Available!\n";
        return;
    }

    double totalAmount = price * quantity;

    // Start transaction
    mysql_query(conn, "START TRANSACTION");

    // Reduce product quantity
    string updateQuery =
        "UPDATE products SET quantity = quantity - " +
        to_string(quantity) +
        " WHERE product_id = " +
        to_string(id);

    if (mysql_query(conn, updateQuery.c_str()))
    {
        cout << "\nFailed to Update Product Quantity!\n";
        cout << mysql_error(conn) << endl;

        mysql_query(conn, "ROLLBACK");
        return;
    }

    // Save sale history
    string saleQuery =
        "INSERT INTO sales "
        "(product_id, quantity_sold, total_amount) VALUES (" +
        to_string(id) + ", " +
        to_string(quantity) + ", " +
        to_string(totalAmount) + ")";

    if (mysql_query(conn, saleQuery.c_str()))
    {
        cout << "\nFailed to Save Sale!\n";
        cout << mysql_error(conn) << endl;

        // Undo the quantity update
        mysql_query(conn, "ROLLBACK");
        return;
    }

    // Save both operations permanently
    if (mysql_query(conn, "COMMIT"))
    {
        cout << "\nFailed to Complete Transaction!\n";
        cout << mysql_error(conn) << endl;

        mysql_query(conn, "ROLLBACK");
        return;
    }

    cout << "\n========== SALE COMPLETED ==========\n";

    cout << "Product ID       : " << id << endl;
    cout << "Quantity Sold    : " << quantity << endl;
    cout << "Price Per Unit   : " << price << endl;
    cout << "Total Amount     : " << totalAmount << endl;

    cout << "\nSale Saved Successfully!\n";
}
void lowStockAlert()
{
    string query =
        "SELECT * FROM products WHERE quantity <= 5";

    if (mysql_query(conn, query.c_str()))
    {
        cout << "\nFailed to Check Low Stock!\n";
        cout << mysql_error(conn) << endl;
        return;
    }

    MYSQL_RES *result = mysql_store_result(conn);

    if (result == NULL)
    {
        cout << "\nFailed to Get Product Data!\n";
        return;
    }

    MYSQL_ROW row;

    // Check if any product was found
    if (mysql_num_rows(result) == 0)
    {
        cout << "\nNo Low Stock Products!\n";

        mysql_free_result(result);
        return;
    }

    cout << "\n========== LOW STOCK ALERT ==========\n";

    cout << "ID\tName\t\tCategory\tQuantity\n";
    cout << "----------------------------------------\n";

    while ((row = mysql_fetch_row(result)))
    {
        cout << row[0] << "\t"
             << row[1] << "\t\t"
             << row[2] << "\t"
             << row[4] << endl;
    }

    mysql_free_result(result);
}

void showDashboard()
{
    cout << "\n====================================\n";
    cout << "       INVENTORY DASHBOARD\n";
    cout << "====================================\n";

    // SQL query to calculate inventory statistics
    string query =
        "SELECT COUNT(*), "
        "COALESCE(SUM(quantity), 0), "
        "COALESCE(SUM(CASE WHEN quantity <= 5 "
        "THEN 1 ELSE 0 END), 0), "
        "COALESCE(SUM(price * quantity), 0) "
        "FROM products";

    // Execute the query
    if (mysql_query(conn, query.c_str()) != 0)
    {
        cout << "Error loading dashboard: "
             << mysql_error(conn) << endl;
        return;
    }

    // Get the query result
    MYSQL_RES* result = mysql_store_result(conn);

    if (result == nullptr)
    {
        cout << "Could not get dashboard data.\n";
        return;
    }

    // Read the calculated values
    MYSQL_ROW row = mysql_fetch_row(result);

    if (row != nullptr)
    {
        cout << "Total Products       : " << row[0] << endl;
        cout << "Total Units in Stock : " << row[1] << endl;
        cout << "Low Stock Products   : " << row[2] << endl;
        cout << "Total Stock Value    : Rs. "
             << row[3] << endl;
    }

    // Free the result from memory
    mysql_free_result(result);

    cout << "====================================\n";
}

void searchProduct()
{
    int choice;

    cout << "\n===== ADVANCED PRODUCT SEARCH =====\n";
    cout << "1. Search by Product ID\n";
    cout << "2. Search by Product Name\n";
    cout << "3. Search by Category\n";
    cout << "4. Show Low Stock Products\n";
    cout << "Enter your choice: ";
    cin >> choice;

    string query;

    if (choice == 1)
    {
        int id;

        cout << "Enter Product ID: ";
        cin >> id;

        query = "SELECT * FROM products WHERE product_id = "
                + to_string(id);
    }
    else if (choice == 2 || choice == 3)
    {
        string searchText;

        cin.ignore(10000, '\n');

        if (choice == 2)
            cout << "Enter Product Name: ";
        else
            cout << "Enter Category: ";

        getline(cin, searchText);

        // Safely handle special characters in the search text
        vector<char> escaped(searchText.length() * 2 + 1);

        unsigned long length = mysql_real_escape_string(
            conn,
            escaped.data(),
            searchText.c_str(),
            searchText.length()
        );

        string safeText(escaped.data(), length);

        if (choice == 2)
        {
            query = "SELECT * FROM products WHERE name LIKE '%"
                    + safeText + "%'";
        }
        else
        {
            query = "SELECT * FROM products WHERE category LIKE '%"
                    + safeText + "%'";
        }
    }
    else if (choice == 4)
    {
        query = "SELECT * FROM products WHERE quantity <= 5";
    }
    else
    {
        cout << "Invalid choice!\n";
        return;
    }

    // Run the search in MySQL
    if (mysql_query(conn, query.c_str()) != 0)
    {
        cout << "Search failed: " << mysql_error(conn) << endl;
        return;
    }

    MYSQL_RES* result = mysql_store_result(conn);

    if (result == nullptr)
    {
        cout << "Could not read search results.\n";
        return;
    }

    MYSQL_ROW row;
    bool found = false;

    cout << "\n===== SEARCH RESULTS =====\n";

    while ((row = mysql_fetch_row(result)) != nullptr)
    {
        found = true;

        cout << "Product ID : " << row[0] << endl;
        cout << "Name       : " << row[1] << endl;
        cout << "Category   : " << row[2] << endl;
        cout << "Price      : Rs. " << row[3] << endl;
        cout << "Quantity   : " << row[4] << endl;
        cout << "--------------------------\n";
    }

    if (!found)
    {
        cout << "No matching products found.\n";
    }

    mysql_free_result(result);
}

void viewSalesHistory()
{
    string query =
        "SELECT sales.sale_id, products.name, "
        "sales.quantity_sold, sales.total_amount, sales.sale_date "
        "FROM sales "
        "JOIN products ON sales.product_id = products.product_id "
        "ORDER BY sales.sale_id DESC";

    if (mysql_query(conn, query.c_str()))
    {
        cout << "\nFailed to Fetch Sales History!\n";
        cout << mysql_error(conn) << endl;
        return;
    }

    MYSQL_RES *result = mysql_store_result(conn);

    if (result == NULL)
    {
        cout << "\nFailed to Get Sales Data!\n";
        return;
    }

    if (mysql_num_rows(result) == 0)
    {
        cout << "\nNo Sales History Available!\n";
        mysql_free_result(result);
        return;
    }

    MYSQL_ROW row;

    cout << "\n=============================================\n";
    cout << "              SALES HISTORY\n";
    cout << "=============================================\n";

    cout << "Sale ID\tProduct\tQuantity\tAmount\t\tDate\n";
    cout << "-------------------------------------------------------------\n";

    while ((row = mysql_fetch_row(result)))
    {
        cout << row[0] << "\t"
             << row[1] << "\t"
             << row[2] << "\t\t"
             << row[3] << "\t\t"
             << row[4] << endl;
    }

    mysql_free_result(result);
}
int main()
{
    conn = connectDatabase();

    if (conn == NULL)
    {
        return 1;
    }

    cout << "\nDatabase Connected Successfully!\n";

    int choice;

    do
    {
        cout << "1. Add Product\n";
        cout << "2. Display All Products\n";
        cout << "3. Search Product\n";
        cout << "4. Update Product\n";
        cout << "5. Delete Product\n";
        cout << "6. Sell Product\n";
        cout << "7. Low Stock Alert\n";
        cout << "8. Undo Last Delete\n";
        cout << "9. Add Customer Order\n";
        cout << "10. Process Next Order\n";
        cout << "11. Sort Products\n";
        cout << "11. Sort Products\n";
        cout << "12. View Sales History\n";
        cout << "13. Exit\n";

        cout << "Enter your choice: ";
        cin >> choice;

        switch (choice)
        {
        case 1:
            addProduct();
            break;

        case 2:
            displayProducts();
            break;

        case 3:
            searchProduct();
            break;

        case 4:
            updateProduct();
            break;

        case 5:
            deleteProduct();
            break;

        case 6:
            sellProduct();
            break;

        case 7:
            lowStockAlert();
            break;

        case 8:
            undoDelete();
            break;

        case 9:
            addOrder();
            break;

        case 10:
            processOrder();
            break;

        case 11:
            sortProducts();
            break;

        case 12:
            viewSalesHistory();
            break;

        case 13:
            cout << "\nThank you for using Smart Inventory Management System!\n";
            break;
        default:
            cout << "\nInvalid Choice! Please try again.\n";
        }

    } while (choice != 13);
    mysql_close(conn);
    return 0;
}