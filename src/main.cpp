#include <iostream>
#include "db/Connection.h"
#include "db/ConnectionPool.h"

int main() {
    /* 测试案例：高并发场景下，使用连接池 vs 不使用连接池
       模拟插入 1000 条数据
    */

    // 1. 不使用连接池（每次都重新连接）
    /*
    clock_t begin = clock();
    for (int i = 0; i < 1000; ++i) {
        Connection conn;
        conn.connect("127.0.0.1", 3307, "root", "123456", "chat_db");
        std::string sql = "INSERT INTO User(name, password, state) VALUES('test_" + std::to_string(i) + "', '123', 'online')";
        conn.update(sql);
    }
    clock_t end = clock();
    std::cout << "不使用连接池消耗时间: " << (end - begin) * 1000 / CLOCKS_PER_SEC << "ms" << std::endl;
    */

    // 2. 使用连接池
    clock_t begin = clock();
    ConnectionPool* cp = ConnectionPool::getInstance();
    
    // 我们启动4个线程来模拟多用户并发
    // (这里只是单线程循环测试，如果要测真正的并发需要 thread，不过这样已经能看出差距了)
    for (int i = 0; i < 1000; ++i) {
        // 这一句是关键：直接从池里拿，不需要填 IP 端口，而且出作用域自动归还
        std::shared_ptr<Connection> sp = cp->getConnection(); 
        
        char sql[1024] = {0};
        sprintf(sql, "INSERT INTO User(name, password, state) VALUES('pool_test_%d', '123', 'online')", i);
        sp->update(sql);
    }
    clock_t end = clock();
    std::cout << "使用连接池消耗时间: " << (end - begin) * 1000 / CLOCKS_PER_SEC << "ms" << std::endl;

    return 0;
}