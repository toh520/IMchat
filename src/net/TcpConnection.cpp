#include "net/TcpConnection.h"
#include "net/Socket.h" // 确保包含 Socket 定义
#include "msg.pb.h"     // [新增] 引入 Protobuf 生成的头文件
#include <iostream>
#include <unistd.h>
#include <cstring>      // memcpy
#include <arpa/inet.h>  // ntohl

TcpConnection::TcpConnection(Epoll* epoll, int fd) 
    : epoll_(epoll),
      socket_(std::make_unique<Socket>(fd)),
      readBuffer_() 
{
    socket_->setNonBlocking();
}

TcpConnection::~TcpConnection() {
    std::cout << "TcpConnection 资源释放，关闭 fd=" << socket_->getFd() << std::endl;
}

void TcpConnection::onRead() {
    int saveErrno = 0;
    // 1. 从 Socket 读取所有数据到 Buffer 中
    ssize_t n = readBuffer_.readFd(socket_->getFd(), &saveErrno);

    if (n > 0) {
        // [核心逻辑] 循环处理 Buffer 中的数据，解决粘包
        while (true) {
            // 第一步：检查 Buffer 里的数据够不够解析出一个包头 (4字节)
            if (readBuffer_.readableBytes() < 4) {
                break; // 数据不够，等待下次读取
            }

            // 第二步：读取包头，获取包体长度
            // peek() 只是看一眼数据，不会移动读指针
            int32_t len;
            // 使用 memcpy 避免字节对齐问题
            memcpy(&len, readBuffer_.peek(), 4);
            // 网络字节序(大端) 转 主机字节序(小端)
            len = ntohl(len);

            // 安全检查：如果长度非常离谱（比如过大），可能是恶意攻击
            if (len < 0 || len > 65536) {
                std::cout << "错误：非法的数据包长度 " << len << "，关闭连接" << std::endl;
                epoll_->updateChannel(socket_->getFd(), EPOLL_CTL_DEL, 0);
                if (closeCallback_) closeCallback_(socket_->getFd());
                return;
            }

            // 第三步：检查 Buffer 剩下的数据够不够一个完整的包体
            // 4 是包头长度，len 是包体长度
            if (readBuffer_.readableBytes() < 4 + len) {
                break; // 数据不够完整，等待下次数据到来
            }

            // --- 数据完整，开始拆包 ---

            // 1. 先移除 4 字节的包头
            readBuffer_.retrieve(4);

            // 2. 取出 len 字节的包体数据
            std::string data = readBuffer_.retrieveAsString(len);

            // 3. 反序列化 Protobuf
            chat::LoginRequest req;
            if (req.ParseFromString(data)) {
                std::cout << "【收到完整数据包】" << std::endl;
                std::cout << "  用户: " << req.username() << std::endl;
                std::cout << "  密码: " << req.password() << std::endl;
                
                // 这里可以添加业务逻辑：验证密码、回复 LoginResponse 等
            } else {
                std::cout << "Protobuf 解析失败！" << std::endl;
            }

            // 继续循环，看看 Buffer 里是否还有下一个包
        }
    } 
    else if (n == 0) {
        std::cout << "客户端断开连接 fd=" << socket_->getFd() << std::endl;
        epoll_->updateChannel(socket_->getFd(), EPOLL_CTL_DEL, 0);
        if (closeCallback_) {
            closeCallback_(socket_->getFd());
        }
    }
    else {
        std::cout << "TcpConnection 读取数据出错！errno=" << saveErrno << std::endl;
    }
}