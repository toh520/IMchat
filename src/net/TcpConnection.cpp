#include "net/TcpConnection.h"
#include "net/Socket.h" 
// [修正] 因为 CMake 包含了 proto 目录，所以直接引用文件名即可，不要加 proto/ 前缀
#include "msg.pb.h" 
#include "server/chatservice.hpp"
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
            // 包头存放整个包的长度
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
            if (len < 4 || len > 65536) { // 最小长度是4 (只有MsgID，没有包体)
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

            // 1. 先移除 4 字节的包头 (Length)
            readBuffer_.retrieve(4);

            // 2. 再解析 4 字节的 MsgID (业务类型)
            int32_t msgid;
            std::string msgidStr = readBuffer_.retrieveAsString(4);
            memcpy(&msgid, msgidStr.data(), 4);
            msgid = ntohl(msgid);

            // 3. 最后取出剩下的数据 (Protobuf 序列化后的数据)
            // 包体总长度 len - 4 (MsgID占用的长度)
            std::string data = readBuffer_.retrieveAsString(len - 4);

            std::cout << "收到数据: MsgID=" << msgid << " DataLen=" << data.size() << std::endl;

            // 4. [关键] 调用业务层进行分发处理
            // 获取对应消息id的处理器
            auto handler = ChatService::instance()->getHandler(msgid);
            
            // 把当前连接对象(shared_ptr)和数据传给业务层
            handler(shared_from_this(), data);
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

// [新增] 按照自定义协议发送数据: 4字节长度 + 4字节MsgID + Data
void TcpConnection::send(int msgid, std::string data) {
    if (socket_->getFd() == -1) return;

    // 1. 计算总长度: MsgID(4字节) + Data长度
    int32_t len = 4 + data.size();
    
    // 2. 将整数转为网络字节序 (大端)
    int32_t len_net = htonl(len);
    int32_t msgid_net = htonl(msgid);

    // 3. 组装发送缓冲区
    std::string sendBuf;
    sendBuf.resize(4 + 4 + data.size());

    // 填入长度
    memcpy(sendBuf.data(), &len_net, 4);
    // 填入MsgID
    memcpy(sendBuf.data() + 4, &msgid_net, 4);
    // 填入数据
    memcpy(sendBuf.data() + 8, data.data(), data.size());

    // 4. 发送 
    this->send(sendBuf);
}

// 发送数据的方法
void TcpConnection::send(std::string msg) {
    // [新增] 加锁保护，防止多线程同时 write 导致数据错乱
    std::lock_guard<std::mutex> lock(sendMutex_);

    if (socket_->getFd() != -1) {
        ssize_t n = write(socket_->getFd(), msg.c_str(), msg.size());
        if (n == -1) {
             std::cout << "TcpConnection 发送数据失败" << std::endl;
        }
    }
}