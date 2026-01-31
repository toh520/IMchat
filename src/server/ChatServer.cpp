#include "server/ChatServer.h"
#include <iostream>
#include <cstring>
#include <functional> // for std::bind
#include <unistd.h>

ChatServer::ChatServer(int port) : port_(port) {
    // 1. 初始化监听 Socket
    listener_ = std::make_unique<Socket>();
    listener_->bind("0.0.0.0", port_);
    listener_->listen();
    listener_->setNonBlocking(); // Epoll 必须非阻塞

    // 2. 初始化 Epoll
    epoll_ = std::make_unique<Epoll>();
    
    // 3. 把监听 Socket 加入 Epoll
    // EPOLLIN: 读事件, EPOLLET: 边缘触发
    epoll_->updateChannel(listener_->getFd(), EPOLL_CTL_ADD, EPOLLIN | EPOLLET);

    std::cout << "ChatServer 初始化完成，监听端口: " << port_ << std::endl;
}

ChatServer::~ChatServer() {
    // 智能指针会自动释放 Socket 和 Epoll，不需要手动 delete
}

void ChatServer::start() {
    std::cout << "ChatServer 服务已启动..." << std::endl;

    while (true) {
        // 等待事件
        auto events = epoll_->poll();

        for (auto& event : events) {
            int fd = event.data.fd;

            // 情况 A: 监听 Socket 有动静 -> 新用户连接
            if (fd == listener_->getFd()) {
                handleNewConnection();
            }
            // 情况 B: 客户端连接有动静 -> 处理数据
            else if (event.events & EPOLLIN) {
                // 从 map 找到对应的连接对象
                if (connections_.count(fd)) {
                    connections_[fd]->onRead();
                } else {
                    // 防御性编程：理论上不该进这里，除非 map 没同步
                    epoll_->updateChannel(fd, EPOLL_CTL_DEL, 0);
                    close(fd);
                }
            }
        }
    }
}

void ChatServer::handleNewConnection() {
    struct sockaddr_in addr;
    int clnt_fd = listener_->accept(&addr);
    
    if (clnt_fd == -1) return;

    // 创建连接对象
    auto conn = std::make_shared<TcpConnection>(epoll_.get(), clnt_fd);

    // [关键] 设置关闭回调
    // 当 TcpConnection 发现客户端断开时，会调用 ChatServer::handleClientDisconnect
    conn->setCloseCallback(std::bind(&ChatServer::handleClientDisconnect, this, std::placeholders::_1));

    // 存入 map
    connections_[clnt_fd] = conn;

    // 加入 Epoll
    epoll_->updateChannel(clnt_fd, EPOLL_CTL_ADD, EPOLLIN | EPOLLET);

    std::cout << "新连接建立 fd=" << clnt_fd << " 当前在线: " << connections_.size() << std::endl;
}

void ChatServer::handleClientDisconnect(int fd) {
    // [新增] 通知业务层处理客户端异常退出 (比如把用户状态改为 offline)
    // 这里需要 ChatService 提供一个处理客户端异常退出的接口
    ChatService::instance()->clientCloseException(connections_[fd]);

    // 从 map 中移除，智能指针引用计数归零 -> 自动析构 TcpConnection
    connections_.erase(fd);
    std::cout << "客户端断开，已回收资源 fd=" << fd << " 当前在线: " << connections_.size() << std::endl;
}