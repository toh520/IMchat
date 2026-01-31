#pragma once
#include <memory> // [关键] 必须包含
#include <functional>
#include <string>
#include "net/Socket.h"
#include "net/Epoll.h"
#include "net/Buffer.h"

// [关键修改] 继承 std::enable_shared_from_this
// 这样我们在成员函数里就能通过 shared_from_this() 拿到管理自己的那个智能指针
class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
    using ptr = std::shared_ptr<TcpConnection>;
    using CloseCallback = std::function<void(int)>;

    TcpConnection(Epoll* epoll, int fd);
    ~TcpConnection();

    void onRead();

    // [新增] 发送数据的方法 (业务层会调用这个)
    // 直接发送 string 数据
    void send(std::string msg);

    void setCloseCallback(const CloseCallback& cb) { closeCallback_ = cb; }

private:
    Epoll* epoll_;
    std::unique_ptr<Socket> socket_;
    Buffer readBuffer_;
    CloseCallback closeCallback_;
};