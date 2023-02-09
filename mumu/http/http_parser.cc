#include "http_parser.h"
#include "http/http.h"
#include "http/http11_parser.h"
#include "http/httpclient_parser.h"
#include "log.h"
#include "config.h"
#include <cstdint>
#include <string.h>

namespace muhui {
namespace http {

static muhui::Logger::ptr g_logger = MUHUI_LOG_NAME("system");

/// request协议解析缓存大小
static muhui::ConfigVar<uint64_t>::ptr g_http_request_buffer_size =
    muhui::Config::Lookup("http.request.buffer.size"
        , (uint64_t)(4 * 1024), "http request buffer size");
/// request协议消息体最大长度
static muhui::ConfigVar<uint64_t>::ptr g_http_request_max_body_size =
    muhui::Config::Lookup("http.request.max.body.size"
        , (uint64_t)(64 * 1024 * 1024ull), "http request max body size");

/// responce协议解析缓存大小
static muhui::ConfigVar<uint64_t>::ptr g_http_responce_buffer_size =
    muhui::Config::Lookup("http.responce.buffer.size"
        , (uint64_t)(4 * 1024), "http responce buffer size");
/// request协议消息体最大长度
static muhui::ConfigVar<uint64_t>::ptr g_http_responce_max_body_size =
    muhui::Config::Lookup("http.responce.max.body.size"
        , (uint64_t)(64 * 1024 * 1024ull), "http responce max body size");

///初始化
static uint64_t s_http_request_buffer_size = 0;
static uint64_t s_http_request_max_body_size = 0;
static uint64_t s_http_responce_buffer_size = 0;
static uint64_t s_http_responce_max_body_size = 0;

/**
 * @brief 注册监听函数
 */
namespace {
struct _SizeIniter {
    _SizeIniter() {
        s_http_request_buffer_size = g_http_request_buffer_size->getValue();
        s_http_request_max_body_size = g_http_request_max_body_size->getValue();
        s_http_responce_buffer_size = g_http_responce_buffer_size->getValue();
        s_http_responce_max_body_size = g_http_responce_max_body_size->getValue();

        //注册监听回调
        g_http_request_buffer_size->addListener([](const uint64_t& ov, const uint64_t& nv){
            s_http_request_buffer_size = nv;
        });
        g_http_request_max_body_size->addListener([](const uint64_t& ov, const uint64_t& nv){
            s_http_request_max_body_size = nv;
        });
        g_http_responce_buffer_size->addListener([](const uint64_t& ov, const uint64_t& nv){
            s_http_responce_buffer_size = nv;
        });
        g_http_responce_max_body_size->addListener([](const uint64_t& ov, const uint64_t& nv){
            s_http_responce_max_body_size = nv;
        });
    }
};
}

//初始化
static _SizeIniter _init;

/**
 * @brief http_parser callback
 * 
 * @param data 
 * @param at 
 * @param length 
 */
void on_request_method (void *data, const char *at, size_t length) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    HttpMethod m = charsToHttpMethod(at);
    if(m == HttpMethod::INVALID_METHOD) {
        MUHUI_LOG_WARN(g_logger) << "invalid http request method: " 
            << std::string(at, length);
        parser->setError(1000);
        return;
    }
    parser->getData()->setMethod(m);
}
void on_request_uri (void *data, const char *at, size_t length) {

}
void on_request_fragment (void *data, const char *at, size_t length) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->getData()->setFragment(std::string(at, length));   
}
void on_request_path (void *data, const char *at, size_t length) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->getData()->setPath(std::string(at, length));
}
void on_request_query (void *data, const char *at, size_t length) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->getData()->setQuery(std::string(at, length));
}
void on_request_version (void *data, const char *at, size_t length) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    uint8_t v = 0;
    if(strncmp(at, "HTTP/1.1", length) == 0) {
        v = 0x11;
    } else if(strncmp(at, "HTTP/1.0", length) == 0) {
        v = 0x10;
    } else {
        MUHUI_LOG_WARN(g_logger) << "invalid http request version: "
            << std::string(at, length);
        parser->setError(1001);
        return;
    }
    parser->getData()->setVersion(v);
}
void on_request_header_done (void *data, const char *at, size_t length) {

}
void on_request_http_field (void *data, const char *field, size_t flen, const char *value, size_t vlen) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    if(flen == 0) {
        MUHUI_LOG_ERROR(g_logger) << "invalid http request field length == 0";
        return;
    }
    parser->getData()->setHeader(std::string(field, flen), std::string(value, vlen));
}


HttpRequestParser::HttpRequestParser() 
    : m_error(0)
{
    m_data.reset(new muhui::http::HttpRequest);
    http_parser_init(&m_parser);
    m_parser.request_method = on_request_method;
    m_parser.request_method = on_request_method;
    m_parser.request_uri    = on_request_uri;
    m_parser.fragment       = on_request_fragment;
    m_parser.request_path   = on_request_path;
    m_parser.query_string   = on_request_query;
    m_parser.http_version   = on_request_version;
    m_parser.header_done    = on_request_header_done;
    m_parser.http_field     = on_request_http_field;
    m_parser.data           = this;
}

size_t HttpRequestParser::execute(char* data, size_t len) {
    size_t offset = http_parser_execute(&m_parser, data, len, 0);
    //将未处理的数据移到缓存的前面，将处理后的数据移走
    memmove(data, data + offset, (len - offset));
    return offset;
}
int HttpRequestParser::isFinished() {
    return http_parser_finish(&m_parser);
}
int HttpRequestParser::hasError() {
    return m_error || http_parser_has_error(&m_parser);
}

uint64_t HttpRequestParser::getcontentLength() {

     return m_data->getHeaderAs<uint64_t>("content-length", 0);
}

uint64_t HttpRequestParser::GetHttpRequestBufferSize() {

     return s_http_request_buffer_size;
}

uint64_t HttpRequestParser::GetHttpRequestMaxBodySize() {
     return s_http_request_max_body_size;
}

/**
 * @brief httpclient_parser callback
 * 
 * @param data 
 * @param at 
 * @param length 
 */
void on_response_reason (void *data, const char *at, size_t length) {
    HttpResponceParser* parser = static_cast<HttpResponceParser*>(data);
    parser->getData()->setReason(std::string(at, length));    
}
void on_response_status (void *data, const char *at, size_t length) {
    HttpResponceParser* parser = static_cast<HttpResponceParser*>(data);
    HttpStatus status = (HttpStatus)(atoi(at)); //atoi str->int
    parser->getData()->setStatus(status);
}
void on_response_chunk (void *data, const char *at, size_t length) {
}
void on_response_version (void *data, const char *at, size_t length) {
    HttpResponceParser* parser = static_cast<HttpResponceParser*>(data);
    uint8_t v = 0;
    if(strncmp(at, "HTTP/1.1", length) == 0) {
        v = 0x11;
    } else if(strncmp(at, "HTTp/1.0", length) == 0) {
        v = 0x10;
    } else {
        MUHUI_LOG_WARN(g_logger) << "invalid http responce version: " 
            << std::string(at, length);
        parser->setError(1001);
        return;
    }
    parser->getData()->setVersion(v);
}
void on_response_header_done(void *data, const char *at, size_t length) {

}
void on_response_last_chunk (void *data, const char *at, size_t length) {

}
void on_response_http_field (void *data, const char *field, size_t flen, const char *value, size_t vlen) {
    HttpResponceParser* parser = static_cast<HttpResponceParser*>(data);
    if(flen == 0) {
        MUHUI_LOG_ERROR(g_logger) << "invalid http responce field length == 0";
        return;
    }
    parser->getData()->setHeader(std::string(field, flen), std::string(value, vlen));
}
HttpResponceParser::HttpResponceParser() 
    : m_error(0)
{
    m_data.reset(new muhui::http::HttpResponce);
    httpclient_parser_init(&m_parser);
    m_parser.reason_phrase = on_response_reason;
    m_parser.status_code   = on_response_status;
    m_parser.chunk_size    = on_response_chunk;
    m_parser.http_version  = on_response_version;
    m_parser.header_done   = on_response_header_done;
    m_parser.last_chunk    = on_response_last_chunk;
    m_parser.http_field    = on_response_http_field;
    m_parser.data          = this;

}

size_t HttpResponceParser::execute(char* data, size_t len, bool chunck) {
    if(chunck) {
        httpclient_parser_init(&m_parser);
    }
    size_t offset = httpclient_parser_execute(&m_parser, data, len, 0);
    memmove(data, data + offset, (len - offset));
    return offset;
}
int HttpResponceParser::isFinished() {
    return httpclient_parser_finish(&m_parser);
}
int HttpResponceParser::hasError() {
    return m_error || httpclient_parser_has_error(&m_parser);
}
uint64_t HttpResponceParser::getcontentLength() {
    return m_data->GetHeaderAs<uint64_t>("content-length", 0);
}
uint64_t HttpResponceParser::GetHttpResponceBufferSize() {
    return s_http_responce_buffer_size;
}

uint64_t HttpResponceParser::GetHttpResponceMaxBodySize() {
    return s_http_responce_max_body_size;
}
} // namespace http
} // namespace muhui