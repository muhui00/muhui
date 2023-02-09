/*****************************************
 * Copyright (C) 2022 * Ltd. All rights reserved.
 *
 * File name   : address.cpp
 * Author      : muhui
 * Created date: 2022-12-07 00:08:24
 * Description : 网络地址的封装(IPv4,Pv6,Unix)实现

 ******************************** ***********/

#include <map>
#include <string>
#include <sys/socket.h>
#include <utility>
#define LOG_TAG "ADDRESS"
#include "address.h"
#include "endian.hh"
#include "log.h"
#include <sstream>
#include <stddef.h>
#include <netdb.h>
#include <ifaddrs.h>

namespace muhui {

static muhui::Logger::ptr g_logger = MUHUI_LOG_NAME("system");

/********************************ADDRESS*****************************************/
//函数模板创建掩码
template<class T>
static T CreateMask(uint32_t bits) {
    return (1 << (sizeof(T) * 8 - bits)) - 1;
}

//统计int类型二进制下有多少个1
template <class T>
static uint32_t CountBytes(T value) {
    uint32_t result = 0;
    for (; value; ++result) {
        value &= value - 1;
    }
    return result;
}

Address::ptr Address::LookupAny(const std::string& host,
            int family, int type, int protocol)
{
    std::vector<Address::ptr> result; 
    if(Lookup(result, host, family, type, protocol)) {
        return result[0];
    }
    return nullptr;
}

std::shared_ptr<IPAddress> Address::LookupAnyIPAddress(const std::string& host,
            int family, int type, int protocol)
{
    std::vector<Address::ptr> result;
    if(Lookup(result, host, family, type, protocol)) {
        for(auto &i : result) {
            //智能指针向子类转换
            IPAddress::ptr v = std::dynamic_pointer_cast<IPAddress>(i);
            if(v) {
                return v;
            }
        }
    }
    return nullptr;
}


/**
 * @brief 通过host地址返回对应条件的所有Address
 * @param[out] result 保存满足条件的Address
 * @param[in] host 域名,服务器名等.举例: www.sylar.top[:80] (方括号为可选内容)
 * @param[in] family 协议族(AF_INT, AF_INT6, AF_UNIX)
 * @param[in] type socketl类型SOCK_STREAM、SOCK_DGRAM 等
 * @param[in] protocol 协议,IPPROTO_TCP、IPPROTO_UDP 等
 * @return 返回是否转换成功
 */
bool Address::Lookup(std::vector<Address::ptr>& result, const std::string& host,
            int family, int type, int protocol)
{
    addrinfo hints, *results, *next;
    hints.ai_flags = 0;
    hints.ai_family = family;
    hints.ai_socktype = type;
    hints.ai_protocol = protocol;
    hints.ai_addrlen = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    //地址
    std::string node;
    //服务
    const char* service = NULL;

    //检查ipv6address service （端口号、ftp、http等）
    //如果地址以 '[' 开头，则可能是 ipv6 地址
    if(!host.empty() && host[0] == '[') {
        //扫描host中']', 成功返回指向该位置的指针
        const char* endipv6 = (const char*)memchr(host.c_str() + 1, ']', host.size() - 1);
        if(endipv6) {
            if(*(endipv6 + 1) == ':') {
                // 如果地址和服务使用 ':' 分隔，则将 service 设为服务名
                service = endipv6 + 2;
            }
            //复制域名(地址)
            node = host.substr(1, endipv6 - host.c_str() - 1);
        }
    }

    //检查node service 不是ipv6地址
    if(node.empty()) {
        service = (const char*)memchr(host.c_str(), ':', host.size());
        if(service) {
            //如果只有一个":", 则为端口号
            if(!memchr(service + 1, ':', host.c_str() + host.size() - service - 1)) {
                //获取域名(地址)
                node = host.substr(0, service - host.c_str());
                ++ service;
            }
        }
    }

    if(node.empty()) {
        node = host;
    }

    //获取地址结构信息，地址node，服务service
    int error = getaddrinfo(node.c_str(), service, &hints, &results);
    if(error) {
        MUHUI_LOG_DEBUG(g_logger) << "Address::Lookup getAddress(" << host << ", "
            << family << ", " << type << ") err=" << error << " errstr=" 
            << gai_strerror(error);
        return false;
    }

    next = results;
    while(next) {
        //创建Address地址，并加入到容器
        result.push_back(Create(next->ai_addr, (socklen_t)next->ai_addrlen));
        next = next->ai_next;
    }

    freeaddrinfo(results);
    return !result.empty();
}

 /**
 * @brief 返回本机所有网卡的<网卡名, 地址, 子网掩码位数>
 * @param[out] result 保存本机所有地址
 * @param[in] family 协议族(AF_INT, AF_INT6, AF_UNIX)
 * @return 是否获取成功
 */
bool Address::GetInterfaceAddresses(std::multimap<std::string
                ,std::pair<Address::ptr, uint32_t> >& result,
                int family)
{
    struct ifaddrs *next, *results;
    //创建一个“struct ifaddrs”结构的链表，一个用于主机上的每个网络接口
    //如果成功，将列表存储在 *IFAP 中并返回 0。如果出错，返回 -1 并设置“errno”
    //*IFAP 中返回的存储是动态分配的，只能通过将其传递给“freeifaddrs”来正确释放。
    if(getifaddrs(&results) != 0) {
        MUHUI_LOG_DEBUG(g_logger) << "Address::GetInterfaceAddresses getifaddrs "
                                     " err="
                                     << errno << " errstr=" << strerror(errno);
        return false;
    }
    try {
        for (next = results; next; next = next->ifa_next)
        {
            Address::ptr addr;
            uint32_t prefix_len = ~0u; // 0xFFFF
            if (family != AF_UNSPEC && family != next->ifa_addr->sa_family)
            {
                continue;
            }
            switch (next->ifa_addr->sa_family) {
                case AF_INET:
                {
                    //创建IPV4地址
                    addr = Create(next->ifa_addr, sizeof(sockaddr_in));
                    //子网掩码
                    uint32_t netmask = ((sockaddr_in *)next->ifa_netmask)->sin_addr.s_addr;
                    prefix_len = CountBytes(netmask);
                }
                break;
                case AF_INET6:
                {
                    //创建IPV6地址
                    addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
                    in6_addr &netmask = ((sockaddr_in6 *)next->ifa_netmask)->sin6_addr;
                    prefix_len = 0;
                    for (int i = 0; i < 16; i++)
                    {
                        prefix_len += CountBytes(netmask.s6_addr[i]);
                    }           
                }
                break;
                default:
                    break;
            }

            if(addr) {
                result.insert(std::make_pair(next->ifa_name, std::make_pair(addr, prefix_len)));
            }
        }
    } catch (...) {
        MUHUI_LOG_ERROR(g_logger) << "Address::GetInterfaceAddresses exception";
        freeifaddrs(results);
        return false;
    }
    freeifaddrs(results);
    return !result.empty();
}

/**
 * @brief 获取指定网卡的地址和子网掩码位数
 * @param[out] result 保存指定网卡所有地址
 * @param[in] iface 网卡名称
 * @param[in] family 协议族(AF_INT, AF_INT6, AF_UNIX)
 * @return 是否获取成功
 */
bool Address::GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t> >&result
                ,const std::string& iface, int family)
{
    if(iface.empty() || iface == "*") {
        if(family == AF_INET || family == AF_UNSPEC) {
            result.push_back(std::make_pair(Address::ptr(new IPv4Address()), 0u));
        }
        if(family == AF_INET6 || family == AF_UNSPEC) {
            result.push_back(std::make_pair(Address::ptr(new IPv6Address()), 0u));
        }
        return true;
    }

    std::multimap<std::string, std::pair<Address::ptr, uint32_t>> results;

    if(!GetInterfaceAddresses(results, family)) {
        return false;;
    }

    auto its = results.equal_range(iface);
    for(; its.first != its.second; ++its.first) {
        result.push_back(its.first->second);
    } 
    return !result.empty();
}

Address::ptr Address::Create(const sockaddr* addr, socklen_t addrlen)
{
    if(addr == nullptr) {
        return nullptr;
    }

    Address::ptr result;
    switch(addr->sa_family) {
    case AF_INET:
        result.reset(new IPv4Address(*(const sockaddr_in*)addr));
        break;
    case AF_INET6:
        result.reset(new IPv6Address(*(const sockaddr_in6*)addr));
        break;
    default:
        result.reset(new UnKnownAddress(*addr));
        break;
    }
    return result;
}

int Address::getFamily() const
{
    return getAddr()->sa_family;
}

std::string Address::toString() const
{
    std::stringstream ss;
    insert(ss);
    return ss.str();
}

bool Address::operator<(const Address& rhs) const
{
    socklen_t minlen = std::min(getAddrLen(), rhs.getAddrLen());
    //比较前minlen个字符
    int result = memcmp(getAddr(), rhs.getAddr(), minlen);
    if(result < 0) {
        return true;
    } else if (result > 0) {
        return  false;
    } else if (getAddrLen() < rhs.getAddrLen()) {
        return true;
    }
    return false;
}
bool Address::operator==(const Address& rhs) const
{
    return getAddrLen() == rhs.getAddrLen()
        && memcmp(getAddr(), rhs.getAddr(), getAddrLen()) == 0;
}

bool Address::operator!=(const Address& rhs) const
{
    return !(*this == rhs);
}

/********************************IPADDRESS*****************************************/
IPAddress::ptr IPAddress::Create(const char* address, uint16_t port)
{
    addrinfo hints, *results;
    memset(&hints, 0, sizeof(addrinfo));

    //地址为数字串
    hints.ai_flags = AI_NUMERICHOST;
    //未定义
    hints.ai_family = AF_UNSPEC;

    //返回对应于给定主机名的包含主机名字和地址信息的addrinfo结构体
    //给定节点和服务，它们标识 Internet 主机和服务，getaddrinfo() 返回一个或多个 addrinfo 结构
    //每个结构包含一个 Inter-可以在调用 bind(2) 或 connect(2) 时指定的网络地址。
    int error = getaddrinfo(address, NULL, &hints, &results);
    if(error) {
        MUHUI_LOG_DEBUG(g_logger) << "IPAddress::Create(" << address
            << ", " << port << ") error" << error 
            << " errno=" << errno << " errstr=" << strerror(errno);
        return nullptr;
    }
    try {
        IPAddress::ptr result = std::dynamic_pointer_cast<IPAddress>(
                Address::Create(results->ai_addr, (socklen_t)results->ai_addrlen));
        if(result) {
            result->setPort(port);
        }
        //释放内存
        freeaddrinfo(results);
        return result;
    } catch (...) {
        freeaddrinfo(results);
        return nullptr;
    };
}
/********************************IPV4ADDRESS*****************************************/
IPv4Address::ptr IPv4Address::Create(const char* address, uint16_t port)
{
    IPv4Address::ptr rt(new IPv4Address);
    rt->m_addr.sin_port = byteswapOnLittleEndian(port);
    //该函数将字符串src转换为af地址族中的网络地址结构，然后将网络地址结构复制到dst中
    int result = inet_pton(AF_INET, address, &rt->m_addr.sin_addr);
    if(result <= 0) {
        MUHUI_LOG_DEBUG(g_logger) << "IPv4Address::Create(" << address << ", "
            << port << ") rt=" << result << " errno=" << errno 
            << " errstr=" << strerror(errno);
        return nullptr;
    }
    return rt;
}

IPv4Address::IPv4Address(const sockaddr_in& address)
{
    m_addr = address;
}
IPv4Address::IPv4Address(uint32_t address, uint16_t port)
{
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = byteswapOnLittleEndian(port);
    m_addr.sin_addr.s_addr = byteswapOnLittleEndian(address);
}

const sockaddr* IPv4Address::getAddr() const
{
    return (sockaddr *)&m_addr;
}
sockaddr* IPv4Address::getAddr()
{
    return (sockaddr *)&m_addr;
}
socklen_t IPv4Address::getAddrLen() const
{
    return sizeof(m_addr);
}
std::ostream& IPv4Address::insert(std::ostream& os) const
{
    uint32_t addr = byteswapOnLittleEndian(m_addr.sin_addr.s_addr);
    os << ((addr >> 24) & 0xff) << "."
        << ((addr >> 16) & 0xff) << "."
        << ((addr >> 8) & 0xff) << "."
        << (addr & 0xff);
    os << ":" << byteswapOnLittleEndian(m_addr.sin_port);
    return os;
}

IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len)
{
    if(prefix_len > 32) {
        return nullptr;
    }
    sockaddr_in baddr(m_addr);
    baddr.sin_addr.s_addr |= byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(baddr));
}
IPAddress::ptr IPv4Address::networdAddress(uint32_t prefix_len)
{
    if(prefix_len > 32) {
        return nullptr;
    }
    sockaddr_in baddr(m_addr);
    baddr.sin_addr.s_addr &= byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(baddr));
}
IPAddress::ptr IPv4Address::subnetMask(uint32_t prefix_len)
{
    sockaddr_in subnet;
    memset(&subnet, 0, sizeof(subnet));
    subnet.sin_family = AF_INET;
    subnet.sin_addr.s_addr = ~byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(subnet));
}
uint32_t IPv4Address::getPort() const
{
    return byteswapOnLittleEndian(m_addr.sin_port);
}
void IPv4Address::setPort(uint16_t v)
{
    m_addr.sin_port = byteswapOnLittleEndian(v);
}


/********************************IPV6ADDRESS*****************************************/
IPv6Address::ptr IPv6Address::Create(const char* address, uint16_t port)
{
    IPv6Address::ptr rt(new IPv6Address);
    rt->m_addr.sin6_port = byteswapOnLittleEndian(port);
    int result = inet_pton(AF_INET6, address, &rt->m_addr.sin6_addr);
    if(result <= 0) {
        MUHUI_LOG_DEBUG(g_logger) << "IPv6Address::Create(" << address << ", "
            << port << ") rt=" << result << " errno=" << errno
            << " errstr=" << strerror(errno);
        return nullptr;
    }
    return rt;
}

IPv6Address::IPv6Address()
{
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin6_family = AF_INET6;
}
IPv6Address::IPv6Address(const sockaddr_in6& address)
{
    m_addr = address;
}
IPv6Address::IPv6Address(const uint8_t address[16], uint16_t port)
{
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin6_family = AF_INET6;
    m_addr.sin6_port = byteswapOnLittleEndian(port);
    memcpy(&m_addr.sin6_addr.s6_addr, address, 16);
}

const sockaddr* IPv6Address::getAddr() const
{
    return (sockaddr *)&m_addr;
}
sockaddr* IPv6Address::getAddr()
{
    return (sockaddr *)&m_addr;
}
socklen_t IPv6Address::getAddrLen() const
{
    return sizeof(m_addr);
}

//TODO
std::ostream& IPv6Address::insert(std::ostream& os) const
{
    //[fe80::215:5dff:fe03:56dd]
    //0010000 000000001 0000000000000000 0011001000111000 1101111111100001
    os << "[";
    uint16_t* addr = (uint16_t*)m_addr.sin6_addr.s6_addr;
    bool used_zeros = false;
    for(int i = 0; i < 8; ++i) {
        if(addr[i] == 0 && !used_zeros) {
            continue;
        }
        if(i && addr[i - 1] == 0 && !used_zeros) {
            os << ":";
            used_zeros = true;
        }
        if(i) {
            os << ":";
        }
        os << std::hex << (int)byteswapOnLittleEndian(addr[i]) << std::dec;
    }
    //压缩格式
    if(!used_zeros && addr[7] == 0) {
        os << "::";
    }
    os << "]:" << byteswapOnLittleEndian(m_addr.sin6_port);
    return os;
}

IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len)
{
    sockaddr_in6 baddr(m_addr);
    baddr.sin6_addr.s6_addr[prefix_len / 8] |=
        CreateMask<uint8_t>(prefix_len % 8);
    for(int i = prefix_len / 8 + 1; i < 16; ++i) {
        baddr.sin6_addr.s6_addr[i] = 0xff;
    }
    return IPv6Address::ptr(new IPv6Address(baddr));
}
IPAddress::ptr IPv6Address::networdAddress(uint32_t prefix_len)
{
    sockaddr_in6 baddr(m_addr);
    baddr.sin6_addr.s6_addr[prefix_len / 8] &=
        CreateMask<uint8_t>(prefix_len % 8);
    for(int i = prefix_len / 8 + 1; i < 16; ++i) {
        baddr.sin6_addr.s6_addr[i] = 0x00;
    }
    return IPv6Address::ptr(new IPv6Address(baddr));
}
IPAddress::ptr IPv6Address::subnetMask(uint32_t prefix_len)
{
    sockaddr_in6 subnet;
    memset(&subnet, 0, sizeof(subnet));
    subnet.sin6_family = AF_INET6;
    subnet.sin6_addr.s6_addr[prefix_len /8] =
        ~CreateMask<uint8_t>(prefix_len % 8);

    for(uint32_t i = 0; i < prefix_len / 8; ++i) {
        subnet.sin6_addr.s6_addr[i] = 0xff;
    }
    return IPv6Address::ptr(new IPv6Address(subnet));
}
uint32_t IPv6Address::getPort() const
{
    return m_addr.sin6_port;
}
void IPv6Address::setPort(uint16_t v)
{
    m_addr.sin6_port = byteswapOnLittleEndian(v);
}

/********************************UNIXADDRESS*****************************************/
static const size_t MAX_PATH_LEN = sizeof(((sockaddr_un *)0)->sun_path) - 1;

UnixAddress::UnixAddress()
{
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    m_length = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
}
UnixAddress::UnixAddress(const std::string& path)
{
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    m_length = path.size() + 1;

    if(!path.empty() && path[0] != '\0') {
        --m_length;
    }

    if(m_length > sizeof(m_addr.sun_path)) {
        throw std::logic_error("path too long");
    }
    memcpy(m_addr.sun_path, path.c_str(), m_length);
    m_length += offsetof(sockaddr_un, sun_path);
}

const sockaddr* UnixAddress::getAddr() const
{
    return (sockaddr *)&m_addr;
}
sockaddr* UnixAddress::getAddr()
{
    return (sockaddr *)&m_addr;
}
socklen_t UnixAddress::getAddrLen() const
{
    return m_length;
}
void UnixAddress::setAddrLen(uint32_t v)
{
    m_length = v;
}
std::string UnixAddress::getPath() const
{
    std::stringstream ss;
    if(m_length > offsetof(sockaddr_un, sun_path)
       && m_addr.sun_path[0] == '\0') {
        ss << "\\0" << std::string(m_addr.sun_path + 1, 
                m_length - offsetof(sockaddr_un, sun_path) - 1);
    } else {
        ss << m_addr.sun_path;
    }
    return ss.str();
}
std::ostream& UnixAddress::insert(std::ostream& os) const
{
    if(m_length > offsetof(sockaddr_un, sun_path) 
       && m_addr.sun_path[0] == '\0') {
         os << "\\0" << std::string(m_addr.sun_path + 1, 
                m_length - offsetof(sockaddr_un, sun_path) - 1);
    }
    return os << m_addr.sun_path;
}
/********************************UNKNOWNADDRESS*****************************************/
UnKnownAddress::UnKnownAddress(int family)
{
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sa_family = family;
}
UnKnownAddress::UnKnownAddress(const sockaddr& addr)
{
    m_addr = addr;
}

const sockaddr* UnKnownAddress::getAddr() const
{
    return (sockaddr *)&m_addr;
}
sockaddr* UnKnownAddress::getAddr() 
{
    return (sockaddr *)&m_addr;
}
socklen_t UnKnownAddress::getAddrLen() const
{
    return sizeof(m_addr);
}
std::ostream& UnKnownAddress::insert(std::ostream& os) const 
{
    os << "[UnKnownAddress family=" << m_addr.sa_family << "]";
    return os;
}

std::ostream& operator<<(std::ostream& os, const Address& addr) {
    return addr.insert(os);
}
}//muhui
