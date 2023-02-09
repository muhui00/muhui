/**
 * @file http_parser.h
 * @author muhui (2571579@qq.com)
 * @brief HTTP解析协议封装
 * @version 0.1
 * @date 2023-02-03
 */
#ifndef __MUHUI_HTTP_HTTP_PARSER_H__
#define __MUHUI_HTTP_HTTP_PARSER_H__
#include "http.h"
#include "httpclient_parser.h"
#include "http11_parser.h"
#include <cstdint>
#include <memory>

namespace muhui {
namespace http {

/**
 * @brief HTTP请求解析类
 */
class HttpRequestParser {
public:
    typedef std::shared_ptr<HttpRequestParser> ptr;
    HttpRequestParser();

    /**
     * @brief 解析协议
     * 
     * @param[in, out] data 协议文本内存
     * @param[in] len 协议文本内存长度
     * @return size_t 返回实际解析的长度， 并且将已解析的数据移除
     */
    size_t execute(char* data, size_t len);
    int isFinished();
    int hasError();

    /**
     * @brief 返回HttpRequest结构体
     */
    HttpRequest::ptr getData() const {return m_data;}
    void setError(int v) {m_error = v;}
    uint64_t getcontentLength();
    const http_parser& getParser() const { return m_parser;}

public:
    /**
     * @brief 获取HttpRequest协议解析的缓存大小
     * 
     * @return uint64_t 
     */
    static uint64_t GetHttpRequestBufferSize();

    /**
     * @brief 返回HttpResponce协议的最大消息体长度
     * 
     * @return uint64_t 
     */
    static uint64_t GetHttpRequestMaxBodySize();

private:
    /// struct http_parser http11_parser.h
    http_parser m_parser;  
    /// HttpRequest 结构
    HttpRequest::ptr m_data;

    /// 错误码
    /// 1000: invalid method
    /// 1001: invalid version
    /// 1002: invalid field
    int m_error;
};

/**
 * @brief HTTP响应解析类
 * 
 */
class HttpResponceParser {
public:
    typedef std::shared_ptr<HttpResponceParser> ptr;
    HttpResponceParser();
    /**
     * @brief 解析协议
     * 
     * @param[in, out] data 协议文本内存
     * @param[in] len 协议文本内存长度
     * @paragraph chunck 是否在解析chunck
     * @return size_t 返回实际解析的长度， 并且将已解析的数据移除
     */
    size_t execute(char* data, size_t len, bool chunck);
    /**
     * @brief 是否解析完成
     * 
     * @return int 
     */
    int isFinished();
    /**
     * @brief 是否有错误
     * 
     * @return int 
     */
    int hasError();

    HttpResponce::ptr getData() const {return m_data;}
    void setError(int v) {m_error = v;}
    /**
     * @brief 返回消息体长度
     * 
     * @return uint64_t 
     */
    uint64_t getcontentLength();
    const httpclient_parser& getParser() const { return m_parser;}

public:
    /**
     * @brief 获取HttpResponce协议解析的缓存大小
     * 
     * @return uint64_t 
     */
    static uint64_t GetHttpResponceBufferSize();

    /**
     * @brief 返回HttpResponce协议的最大消息体长度
     * 
     * @return uint64_t 
     */
    static uint64_t GetHttpResponceMaxBodySize();

private:
    /// struct http_parser http11_parser.h
    httpclient_parser m_parser;  
    /// HttpRequest 结构
    HttpResponce::ptr m_data;

    /// 错误码
    /// 1001: invalid version
    /// 1002: invalid field
    int m_error;
};
} // namespace http
} // namespace muhui

#endif