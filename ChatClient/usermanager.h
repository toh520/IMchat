#ifndef USERMANAGER_H
#define USERMANAGER_H

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include <functional>
#include "msg.pb.h" // protoc 生成的头文件

// 消息回调类型
using MsgHandler = std::function<void(const QByteArray &data)>;

class UserManager : public QObject
{
    Q_OBJECT
public:
    static UserManager& instance();

    // 连接服务器
    void connectToServer(const QString &ip, quint16 port);

    // 发送消息通用接口
    // msgid: 消息ID
    // data: 序列化后的 Protobuf 数据
    void send(int msgid, const std::string &data);

    // 注册特定消息ID的回调
    void registerHandler(int msgid, MsgHandler handler);

    // 获取 socket 指针 (用于判断连接状态)
    QTcpSocket* socket() { return m_socket; }

    quint32 getCurrentUid() const { return m_currentUid; }
    void setCurrentUid(quint32 uid) { m_currentUid = uid; }

signals:
    // 连接成功信号
    void connected();
    // 连接断开信号
    void disconnected();
    
private slots:
    void onReadyRead();

private:
    explicit UserManager(QObject *parent = nullptr);
    
    QTcpSocket *m_socket;
    
    // 处理 TCP 粘包所需的缓存
    QByteArray m_buffer;
    
    // [新增] 待发送消息队列，用于解决未连接时发送消息丢失的问题
    std::vector<QByteArray> m_sendQueue;

    // 当前登录的用户ID
    quint32 m_currentUid = 0;

    // 消息回调映射表
    std::map<int, MsgHandler> m_handlers;
};

#endif // USERMANAGER_H