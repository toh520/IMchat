#include "server/model/offlinemessagemodel.hpp"
#include "db/ConnectionPool.h"
#include <iostream>

void OfflineMsgModel::insert(int userid, std::string msg) {
    // 1. 组装 SQL
    // 注意：这里没有防 SQL 注入，生产环境应该用预编译语句，但作为 C++ 项目演示核心原理暂可忽略
    char sql[1024] = {0};
    sprintf(sql, "INSERT INTO OfflineMessage(userid, message) VALUES(%d, '%s')", 
            userid, msg.c_str());

    ConnectionPool* cp = ConnectionPool::getInstance();
    std::shared_ptr<Connection> sp = cp->getConnection();
    if (sp) {
        sp->update(sql);
    }
}

void OfflineMsgModel::remove(int userid) {
    char sql[1024] = {0};
    sprintf(sql, "DELETE FROM OfflineMessage WHERE userid=%d", userid);

    ConnectionPool* cp = ConnectionPool::getInstance();
    std::shared_ptr<Connection> sp = cp->getConnection();
    if (sp) {
        sp->update(sql);
    }
}

std::vector<std::string> OfflineMsgModel::query(int userid) {
    char sql[1024] = {0};
    sprintf(sql, "SELECT message FROM OfflineMessage WHERE userid = %d", userid);

    std::vector<std::string> vec;
    ConnectionPool* cp = ConnectionPool::getInstance();
    std::shared_ptr<Connection> sp = cp->getConnection();

    if (sp) {
        MYSQL_RES* res = sp->query(sql);
        if (res != nullptr) {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr) {
                vec.push_back(row[0]);
            }
            mysql_free_result(res);
        }
    }
    return vec;
}