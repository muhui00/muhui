/**
 * @file tcp_server.h
 * @author muhui (2571579302@qq.com)
 * @brief TCP服务器封装
 * @version 0.1
 * @date 2023-02-03
 */

#include <cstdint>
#include <stdint.h>
#include <memory>
#include <functional>
#include <vector>
#include "iomanager.h"
#include "socket.h"
#include "address.h"
#include "noncopyable.h"
#include "config.h"


namespace muhui {

/**
 * @brief TCP服务器配置类
 * @pre 模板特化
 */
struct TcpServerConf{
    typedef std::shared_ptr<TcpServerConf> ptr;
    std::vector<std::string> address;
    int keepalive = 0;
    int timeout = 1000 * 2 * 60;
    int ssl = 0;
    std::string id;
    /// 服务器类型，http, ws, rock
    std::string type = "http";
    std::string name;
    std::string cert_file;
    std::string key_file;
    std::string accept_worker;
    std::string io_worker;
    std::string process_worker;
    std::map<std::string, std::string> args;

    bool isValid() const {
        return !address.empty();
    }

    bool operator==(const TcpServerConf& oth) const {
        return address == oth.address
            && keepalive == oth.keepalive
            && timeout == oth.timeout
            && name == oth.name
            && ssl == oth.ssl
            && cert_file == oth.cert_file
            && key_file == oth.key_file
            && accept_worker == oth.accept_worker
            && io_worker == oth.io_worker
            && process_worker == oth.process_worker
            && args == oth.args
            && id == oth.id
            && type == oth.type;
    } 
};

template<>
class LexcalCast<std::string, TcpServerConf> {
public:
    TcpServerConf operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        TcpServerConf conf;
        conf.id = node["id"].as<std::string>(conf.id);
        conf.type = node["type"].as<std::string>(conf.type);
        conf.keepalive = node["keepalive"].as<int>(conf.keepalive);
        conf.timeout = node["timeout"].as<int>(conf.timeout);
        conf.name = node["name"].as<std::string>(conf.name);
        conf.ssl = node["ssl"].as<int>(conf.ssl);
        conf.cert_file = node["cert_file"].as<std::string>(conf.cert_file);
        conf.key_file = node["key_file"].as<std::string>(conf.key_file);
        conf.accept_worker = node["accept_worker"].as<std::string>();
        conf.io_worker = node["io_worker"].as<std::string>();
        conf.process_worker = node["process_worker"].as<std::string>();
        conf.args = LexcalCast<std::string
            ,std::map<std::string, std::string> >()(node["args"].as<std::string>(""));
        if(node["address"].IsDefined()) {
            for(size_t i = 0; i < node["address"].size(); ++i) {
                conf.address.push_back(node["address"][i].as<std::string>());
            }
        }
        return conf;
    }
};

template<>
class LexcalCast<TcpServerConf, std::string> {
public:
    std::string operator()(const TcpServerConf& conf) {
        YAML::Node node;
        node["id"] = conf.id;
        node["type"] = conf.type;
        node["name"] = conf.name;
        node["keepalive"] = conf.keepalive;
        node["timeout"] = conf.timeout;
        node["ssl"] = conf.ssl;
        node["cert_file"] = conf.cert_file;
        node["key_file"] = conf.key_file;
        node["accept_worker"] = conf.accept_worker;
        node["io_worker"] = conf.io_worker;
        node["process_worker"] = conf.process_worker;
        node["args"] = YAML::Load(LexcalCast<std::map<std::string, std::string>
            , std::string>()(conf.args));
        for(auto& i : conf.address) {
            node["address"].push_back(i);
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/**
 * @brief TCP服务器封装
 */
class TcpServer : public std::enable_shared_from_this<TcpServer>
    , Noncopyable {
public:
    typedef std::shared_ptr<TcpServer> ptr;
    /**
     * @brief Construct a new Tcp Server object
     * 
     * @param[in] worker 
     * @param[in] io_worker 
     * @param[in] acceptWorker 
     */
    TcpServer(IOManager* worker = IOManager::GetThis()
            , IOManager* io_worker = IOManager::GetThis()
            , IOManager* acceptWorker = IOManager::GetThis());
    /**
     * @brief Destroy the Tcp Server object
     */
    virtual ~TcpServer();
    /**
     * @brief 绑定地址
     * @param[in] addr  
     * @param ssl 
     * @return true 
     * @return false 
     */
    virtual bool bind(Address::ptr addr, bool ssl = false);
    /**
     * @brief 绑定地址数组
     * @param addrs 需要绑定的地址数组
     * @param fails 绑定失败的地址数组
     * @param ssl 
     * @return true 
     * @return false 
     */
    virtual bool bind(const std::vector<Address::ptr>& addrs,
                std::vector<Address::ptr> fails, bool ssl = false);
    
    /**
     * @brief 启动服务
     * @pre 需要bind成功之后执行
     * @return true 
     * @return false 
     */
    virtual bool start();

    /**
     * @brief 停止服务
     */
    virtual void stop();

    /**
     * @brief Get the Recv Time Out object
     * 
     * @return uint64_t 
     */
    uint64_t getRecvTimeOut() const {return m_recvTimeout;}
    void setRecvTimeOut(uint64_t v) {m_recvTimeout = v;}
    std::string getName() const {return m_name;}
    void setName (std::string name) {m_name = name;}
    bool isStop() const {return m_isStop;}

    /**
     * @brief 获取TCP服务器配置
     * @return TcpServerConf::ptr 
     */
    TcpServerConf::ptr getConf() const {return m_conf;}

    /**
     * @brief 设置TCP服务器配置
     * @param v 
     */
    void setConf(TcpServerConf::ptr v) {m_conf = v;}

    /**
     * @brief 将TCP服务器信息转化为string类型
     * 
     * @param prefix 
     * @return std::string 
     */
    virtual std::string toString(const std::string& prefix = "");

    /**
     * @brief 获取监听socket数组
     * @return std::vector<Socket::ptr> 
     */
    std::vector<Socket::ptr> getSocks() {return m_socks;}
protected:
    /**
     * @brief 处理新连接的Socket类
     * @pre 子类继承重写
     * @param client 
     */
    virtual void handleCilent(Socket::ptr client);

    /**
     * @brief 开始接受连接
     * @param sock 
     */
    virtual void startAccept(Socket::ptr sock);
protected:
    /// 监听Socket数组
    std::vector<Socket::ptr> m_socks;
    /// 新连接的Socket工作的调度器
    IOManager* m_worker;
    IOManager* m_ioWorker;
    /// 服务器Socket接收连接的调度器
    IOManager* m_acceptWorker;
    /// 接收超时时间(毫秒)
    uint64_t m_recvTimeout;
    /// 服务器名称
    std::string m_name;
    /// 服务器类型
    std::string m_type = "tcp";
    /// 服务是否停止
    bool m_isStop;

    bool m_ssl = false;
    ///TCP服务器配置
    TcpServerConf::ptr m_conf;  
}; //class TcpServer
} // namespace muhui

