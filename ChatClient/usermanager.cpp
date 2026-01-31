#include "usermanager.h"
#include <QtEndian> // qToBigEndian
#include <QDataStream>

UserManager& UserManager::instance()
{
    static UserManager instance;
    return instance;
}

UserManager::UserManager(QObject *parent) : QObject(parent)
{
    m_socket = new QTcpSocket(this);

    // [修改] 连接 lambda，处理连接成功后的队列发送
    connect(m_socket, &QTcpSocket::connected, [this]() {
        emit connected(); // 转发信号
        
        // 检查队列是否有未发送的数据
        if (!m_sendQueue.empty()) {
            qDebug() << "[UserManager] Connection established. Flushing queue size:" << m_sendQueue.size();
            for (const auto &packet : m_sendQueue) {
                m_socket->write(packet);
            }
            m_socket->flush();
            m_sendQueue.clear();
        }
    });
    
    connect(m_socket, &QTcpSocket::disconnected, this, &UserManager::disconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &UserManager::onReadyRead);
}

void UserManager::connectToServer(const QString &ip, quint16 port)
{
    // 如果之前连着，先断开
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->disconnectFromHost();
    }
    m_socket->connectToHost(ip, port);
}

void UserManager::registerHandler(int msgid, MsgHandler handler)
{
    m_handlers[msgid] = handler;
}

// 核心发送逻辑：封装 Length(4) + MsgID(4) + Data
void UserManager::send(int msgid, const std::string &data)
{
    // [修复] 不要在发送时强制检查 ConnectedState。
    // 因为 connectToHost 是异步的，点击登录瞬间状态是 Connecting。
    // QTcpSocket 允许先 write 数据进入缓冲区，等连接建立后自动发送。
    // if (m_socket->state() != QAbstractSocket::ConnectedState) return;

    // [调试] 打印发送日志，确保函数被调用
    qDebug() << "[UserManager] Sending msgid:" << msgid << " data size:" << data.size();

    QByteArray packet;
    QDataStream out(&packet, QIODevice::WriteOnly);
    
    // 设置为大端字节序 (网络字节序)
    out.setByteOrder(QDataStream::BigEndian);

    // 1. 计算包长: MsgID(4) + Data.size()
    // 注意：这里的 Length 仅指包体的长度，还是指 包头+包体？
    // 我们的后端 TcpConnection.cpp 里是：
    // int32_t len = 4 + data.size();  // 包长记录的是 “MsgID长度 + Data长度”
    // 也就是说，Header本身的4字节是不算在 len 里的，但 readBuffer 是先读4字节得到 len，再读 len 字节
    // 后端代码检查: 
    // memcpy(&len, readBuffer_.peek(), 4);
    // if (readBuffer_.readableBytes() < 4 + len) ...
    // 所以协议是：[Length(4bytes)] + [MsgID(4bytes) + Data(N bytes)]
    // Length 的值 = 4 + N

    uint32_t length = 4 + data.size();
    
    // 写入长度 (4字节)
    out << (quint32)length;
    // 写入 MsgID (4字节)
    out << (quint32)msgid;

    // 写入 Data
    // writeRawData 不会加前缀长度，直接写字节
    out.writeRawData(data.data(), data.size());

    // [逻辑更新] 
    // 如果已连接，直接发送
    // 如果未连接，加入队列，等待 connected 信号触发时发送
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        qint64 bytesWritten = m_socket->write(packet);
        qDebug() << "[UserManager] write() returned:" << bytesWritten << " packet size:" << packet.size();
        m_socket->flush();
    } else {
        qDebug() << "[UserManager] Socket not connected. Queueing packet size:" << packet.size();
        m_sendQueue.push_back(packet);
    }
}

// 核心接收逻辑：拆包
void UserManager::onReadyRead()
{
    // 把新来的数据追加到缓冲区
    m_buffer.append(m_socket->readAll());

    while (true) {
        // 1. 既然包头是 4 字节，缓冲区必须大于等于 4
        if (m_buffer.size() < 4) {
            break;
        }

        // 2. 读出包头记录的长度
        QDataStream stream(m_buffer);
        stream.setByteOrder(QDataStream::BigEndian);
        
        quint32 len;
        stream >> len;

        // 3. 检查缓冲区剩下的数据够不够 len
        // m_buffer.size() 是总长度 (包含开头的4字节)
        // 我们需要的数据长度是 4(Length本身) + len(MsgID+Data)
        if (static_cast<quint32>(m_buffer.size()) < 4 + len) {
            // 数据不够，等下次
            break; 
        }

        // --- 数据完整，开始处理 ---
        
        // 4. 读取 MsgID (4字节)
        // QDataStream 会自动往后移指针
        quint32 msgid;
        stream >> msgid;

        // 5. 读取 Data
        // len 包含了 MsgID 的 4 字节，所以真实数据长度是 len - 4
        int dataLen = len - 4;
        
        // 我们不能用 stream 直接读 std::string，需要手动提取
        // 这里比较麻烦的是 QDataStream 很难只读一段而不影响原来 buffer
        // 我们直接操作 QByteArray
        QByteArray dataPart = m_buffer.mid(8, dataLen); // 从第8字节开始 (4+4)，读 dataLen 长度

        // 6. 移除已处理的数据
        m_buffer.remove(0, 4 + len);

        // 7. 分发回调
        auto it = m_handlers.find(msgid);
        if (it != m_handlers.end()) {
            it->second(dataPart);
        }
    }
}
