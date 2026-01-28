#pragma once
#include <memory> // std::enable_shared_from_this, std::shared_ptr
#include "Socket.h"
#include "Epoll.h"
#include "Buffer.h"
#include <functional> // std::function

class TcpConnection {
public:
    //以此类作为指针类型，方便管理
    using ptr = std::shared_ptr<TcpConnection>;

    // [新增] 定义回调函数类型：参数是 int (socket fd)，返回值 void
    using CloseCallback = std::function<void(int)>;

    // 构造函数：传入 Epoll 指针和客户端的 Socket fd
    TcpConnection(Epoll* epoll, int fd);
    ~TcpConnection();

    // 唯一的 public 接口：连接建立后的回调
    // 目前我们还没有完整的 EventLoop，所以这个函数主要用来测试
    void onRead(); 

    // [新增] 设置关闭连接时的回调函数
    void setCloseCallback(const CloseCallback& cb) { closeCallback_ = cb; }

private:
    Epoll* epoll_;          // 持有 Epoll 指针，为了能把自己从 Epoll 中移除
    std::unique_ptr<Socket> socket_; // 独占管理 Socket 资源
    
    // [核心] 专属的输入缓冲区
    // 只要 TcpConnection 对象还在，这个 Buffer 就在，数据就不会丢
    Buffer readBuffer_; 

    // [新增] 保存回调函数
    CloseCallback closeCallback_;
};