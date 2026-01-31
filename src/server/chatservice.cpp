#include "server/chatservice.hpp"
#include "public.hpp"
#include <iostream>

using namespace std;
using namespace chat; // protobuf 命名空间

ChatService* ChatService::instance() {
    static ChatService service;
    return &service;
}

// 注册消息以及对应的Handler回调操作
ChatService::ChatService() {
    // 用户注册业务管理
    // 当收到 REG_MSG (注册) 消息时，绑定到 ChatService::reg 方法
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, std::placeholders::_1, std::placeholders::_2)});
    
    // 用户登录业务管理
    // 当收到 LOGIN_MSG (登录) 消息时，绑定到 ChatService::login 方法
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, std::placeholders::_1, std::placeholders::_2)});
}

// 获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid) {
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end()) {
        // 返回一个默认的空处理器，防止崩溃，并打印错误日志
        return [=](const std::shared_ptr<TcpConnection>& conn, std::string& data) {
            cout << "msgid:" << msgid << " can not find handler!" << endl;
        };
    } else {
        return _msgHandlerMap[msgid];
    }
}

// 处理注册业务
void ChatService::reg(const std::shared_ptr<TcpConnection>& conn, std::string& data) {
    RegRequest req;
    // 1. 反序列化: 把网络层传来的 string 数据转成 RegRequest 对象
    if (req.ParseFromString(data)) {
        string name = req.username();
        string pwd = req.password();

        User user;
        user.setName(name);
        user.setPwd(pwd);
        
        // 2. 真正操作数据库 (Model层)
        bool state = _userModel.insert(user);
        
        // 3. 构建响应
        RegResponse resp;
        if (state) {
            // 注册成功
            resp.set_success(true);
            resp.set_uid(user.getId()); // 这是一个亮点，注册成功直接返回ID
            resp.set_msg("注册成功");
            cout << "用户注册成功: " << name << " ID: " << user.getId() << endl;
        } else {
            // 注册失败
            resp.set_success(false);
            resp.set_msg("注册失败，用户名可能已存在");
            cout << "用户注册失败: " << name << endl;
        }

        // 4. 序列化并发送回去 (目前我们只打印，稍后去 TcpConnection 实现 send)
        string send_str;
        resp.SerializeToString(&send_str);
        
        // TODO: 这里需要 TcpConnection 提供一个 send 方法来发送带 MsgID 的包
        // 目前先模拟打印，稍后我们在 TcpConnection 补充 send 方法
        // conn->send(REG_MSG_ACK, send_str); 
        cout << "向客户端发送注册响应 (Size: " << send_str.size() << ")" << endl;
    }
}

// 处理登录业务
void ChatService::login(const std::shared_ptr<TcpConnection>& conn, std::string& data) {
    LoginRequest req;
    if (req.ParseFromString(data)) {
        string name = req.username();
        string pwd = req.password();

        cout << "收到登录请求: user=" << name << " pwd=" << pwd << endl;

        // 这里还需要补全 UserModel 的查询逻辑（目前只有 query(int id)，需要 query(string name)）
        // 为了跑通流程，我们先模拟一个假登录
        
        LoginResponse resp;
        resp.set_success(false);
        resp.set_msg("登录功能开发中...");
        
        string send_str;
        resp.SerializeToString(&send_str);
        // conn->send(LOGIN_MSG_ACK, send_str);
        cout << "向客户端发送登录响应 (Size: " << send_str.size() << ")" << endl;
    }
}