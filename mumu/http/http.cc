#include "http.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <strings.h>
#include <utility>

namespace muhui {
namespace http {

/**
 * @brief 将字符串方法名转化为HTTP方法枚举
 * @param[in] m HTTP方法
 * @return HttpMethod
 */
HttpMethod StringToHttpMethod(const std::string& m) {
#define XX(num, name, string)                                                  \
    if (strcmp(#string, m.c_str()) == 0) {                                     \
        return HttpMethod::name;                                               \
    }
    HTTP_METHOD_MAP(XX);
#undef XX
    return HttpMethod::INVALID_METHOD;
}

/**
 * @brief 将字符串指针转化为HTTP方法枚举
 * @param[in] m HTTP方法
 * @return HttpMethod
 */
HttpMethod charsToHttpMethod(const char* m) {
#define XX(num, name, string)                                                  \
    if (strncmp(#string, m, strlen(#string)) == 0) {                           \
        return HttpMethod::name;                                               \
    }
    HTTP_METHOD_MAP(XX);
#undef XX
    return HttpMethod::INVALID_METHOD;
}

static const char* s_method_string[] = {
#define XX(num, name, string) #string,
    HTTP_METHOD_MAP(XX)
#undef XX
};

/**
 * @brief 将HTTP方法转化为字符串
 *
 * @param[in] m
 * @return const char*
 */
const char* HttpMethodToString(const HttpMethod& m) {
    uint32_t idx = (uint32_t)m;
    if (idx >= (sizeof(s_method_string) / sizeof(s_method_string[0]))) {
        return "<unknown>";
    }
    return s_method_string[idx];
}

/**
 * @brief 将HTTP状态转化为字符串
 * @param[in] s
 * @return const char*
 */
const char* HttpstatusToString(const HttpStatus& s) {
    switch (s) {
#define XX(code, name, desc)                                                   \
    case HttpStatus::name:                                                     \
        return #desc;
        HTTP_STATUS_MAP(XX);
#undef XX
    default:
        return "<unknown>";
    }
}

/**
 * @brief 忽略大小写比较仿函数
 */
bool CaseInsensitiveLess::operator()(const std::string& lhs,
                                     const std::string& rhs) const {
    return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
}

/**
 * @brief Construct a new Http Request object
 * @param version
 * @param close 是否为keepalive
 */
HttpRequest::HttpRequest(uint8_t version, bool close)
    : m_method(HttpMethod::GET),
      m_version(version),
      m_close(close),
      m_websocket(false),
      m_parserParamFlag(0),
      m_path("/") {}

std::string HttpRequest::getHeader(const std::string& key,
                                   const std::string& def) const {
    auto it = m_headers.find(key);
    return it == m_headers.end() ? def : it->second;
}
std::string HttpRequest::getParam(const std::string& key,
                                  const std::string& def) const {
    auto it = m_params.find(key);
    return it == m_headers.end() ? def : it->second;
}
std::string HttpRequest::getCookie(const std::string& key,
                                   const std::string& def) const {
    auto it = m_cookies.find(key);
    return it == m_headers.end() ? def : it->second;
}

void HttpRequest::setHeader(const std::string& key, const std::string& val) {
    m_headers[key] = val;
}
void HttpRequest::setParam(const std::string& key, const std::string& val) {
    m_params[key] = val;
}
void HttpRequest::setCookie(const std::string& key, const std::string& val) {
    m_cookies[key] = val;
}

void HttpRequest::delHeader(const std::string& key) { m_headers.erase(key); }
void HttpRequest::delParam(const std::string& key) { m_params.erase(key); }
void HttpRequest::delCookie(const std::string& key) { m_cookies.erase(key); }

bool HttpRequest::hasHeader(const std::string& key, std::string* val) {
    auto it = m_headers.find(key);
    if (it == m_headers.end()) {
        return false;
    }
    if (val) {
        *val = it->second;
    }
    return true;
}
bool HttpRequest::hasParam(const std::string& key, std::string* val) {
    auto it = m_params.find(key);
    if (it == m_params.end()) {
        return false;
    }
    if (val) {
        *val = it->second;
    }
    return true;
}
bool HttpRequest::hasCookie(const std::string& key, std::string* val) {
    auto it = m_cookies.find(key);
    if (it == m_cookies.end()) {
        return false;
    }
    if (val) {
        *val = it->second;
    }
    return true;
}

std::string HttpRequest::toString() const {
    std::stringstream ss;
    dump(ss);
    return ss.str();
}
std::ostream& HttpRequest::dump(std::ostream& os) const {
    // GET /uri HTTP/1.1
    // HOST: www.baidu.com
    //
    //
    os << HttpMethodToString(m_method) << " " << m_path
       << (m_query.empty() ? "" : "?") << m_query
       << (m_fragment.empty() ? "" : "#") << m_fragment << " HTTP/"
       << ((uint32_t)(m_version >> 4)) << "." << ((uint32_t)(m_version & 0x0F))
       << "\r\n";
    if (!m_websocket) {
        os << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";
    }
    for (auto& i : m_headers) {
        if (!m_websocket && strcasecmp(i.first.c_str(), "connection") == 0) {
            continue;
        }
        os << i.first << ": " << i.second << "\r\n";
    }
    if (!m_body.empty()) {
        os << "connect-length: " << m_body.size() << "\r\n\r\n" << m_body;
    } else {
        os << "\r\n";
    }
    return os;
}
#if 0
void HttpRequest::init() {
    std::string conn = getHeader("connection");
    if(!conn.empty()) {
        //忽略大小写
        if(strcasecmp(conn.c_str(), "keep-alive") == 0) {
            m_close = false;
        } else {
            m_close = true;
        }
    }
}
void HttpRequest::initParam() {
    initQueryParam();
    initBodyParam();
    initCookies();
}
void HttpRequest::initQueryParam() {
    if(m_parserParamFlag & 0x1) {
        return;
    }
#define PARSE_PARAM(str, m, flag, trim) \
    size_t pos = 0; \
    do { \
        size_t last = pos; \
        //返回字符在字符串内的位置 
        pos = str.find('=', pos); \
        if(pos == std::string::npos) { \
            //不在字符串内
            break; \
        } \
        size_t key = pos; \
        pos = str.find(flag, pos); \
        m.insert(std::make_pair(T1 &&x, T2 &&y))
    
    }while (true);

}
void HttpRequest::initBodyParam() {

}
void HttpRequest::initCookies() {

}
#endif
HttpResponce::HttpResponce(uint8_t version, bool close)
    : m_status(HttpStatus::OK),
      m_version(version),
      m_close(close),
      m_websocket(false) {}
std::string HttpResponce::getHeader(const std::string& key,
                                    const std::string& def) const {
    auto it = m_headers.find(key);
    return it == m_headers.end() ? def : it->second;
}
void HttpResponce::setHeader(const std::string& key, const std::string& val) {
    m_headers[key] = val;
}
void HttpResponce::delHeader(const std::string& key) { m_headers.erase(key); }
std::string HttpResponce::toString() const {
    std::stringstream ss;
    dump(ss);
    return ss.str();
}
std::ostream& HttpResponce::dump(std::ostream& os) const {
    os << "HTTP/" << ((uint32_t)m_version >> 4) << "."
       << ((uint32_t)m_version & 0x0f) << " " << (uint32_t)m_status << " "
       << (m_reason.empty() ? HttpstatusToString(m_status) : m_reason)
       << "\r\n";
    for (auto& i : m_headers) {
        if (!m_websocket && strcasecmp(i.first.c_str(), "connection") == 0) {
            continue;
        }
        os << i.first << ": " << i.second << "\r\n";
    }
    for (auto& i : m_cookies) {
        os << "Set-Cookie: " << i << "\r\n";
    }
    if (!m_websocket) {
        os << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";
    }
    if (!m_body.empty()) {
        os << "content-length: " << m_body.size() << "\r\n\r\n" << m_body;
    } else {
        os << "\r\n";
    }
    return os;
}
void HttpResponce::setRedirect(const std::string& uri) {
    m_status = HttpStatus::FOUND;
    setHeader("Location", uri);
}
void HttpResponce::setCookie(const std::string& key,
                             const std::string& val,
                             time_t expired,
                             const std::string& path,
                             const std::string& domain,
                             bool secure) {
    std::stringstream ss;
    ss << key << "=" << val;
}
std::ostream& operator<<(std::ostream& os, const HttpRequest& req) {
    return req.dump(os);
}
std::ostream& operator<<(std::ostream& os, const HttpResponce& rsp) {
    return rsp.dump(os);
}
} // namespace http
} // namespace muhui