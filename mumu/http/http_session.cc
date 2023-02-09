#include "http_session.h"
#include "http/http.h"
#include "http_parser.h"
#include "socket.h"
#include "streams/socket_stream.h"
#include <cstdint>
#include <cstring>
#include <sstream>

namespace muhui {
namespace http {

HttpSession::HttpSession(Socket::ptr socket, bool owner)
    : SocketStream(socket, owner) {}

HttpRequest::ptr HttpSession::recvRequest() {
    HttpRequestParser::ptr parser(new HttpRequestParser);
    //HTTP协议解析缓存大小
    uint64_t buff_size = HttpRequestParser::GetHttpRequestBufferSize();
    //uint64_t buff_size = 100;
    std::shared_ptr<char> buffer(new char[buff_size], [](char* ptr){
        delete[] ptr;
    });
    char* data = buffer.get();
    //偏移量
    int offset  = 0;
    //解析HTTP协议
    do {
        int len = read(data + offset, buff_size - offset);
        if(len < 0) {
            close();
            return nullptr;
        }
        //解析数据的长度
        len += offset;
        //实际解析数据长度
        size_t nparser = parser->execute(data, len);
        if(parser->hasError()) {
            close();
            return nullptr;
        }
        //未解析数据长度
        offset = len - nparser;
        if(offset == (int)buff_size) {
            close();
            return nullptr;
        }
        //解析结束
        if(parser->isFinished()) {
            break;
        }

    }while (true); 
    //解析content
     int length = parser->getcontentLength();
     if(length > 0) {
        std::string body;
        //预留空间
        body.resize(length);
        int len = 0;
        if(length >= offset) {
            memcpy(&body[0], data, offset);
            len = offset;
        } else {
            memcpy(&body[0], data, length);
        }
        //未读取的数据
        length -= offset;
        if(length > 0) {
            if(readFixSize(&body[len], length) <= 0) {
                close();
                return nullptr;
            }
        }
        parser->getData()->setBody(body);
     }
     return parser->getData();
}

int HttpSession::sendResponse(HttpResponce::ptr rsp) {
    std::stringstream ss;
    ss << *rsp;
    std::string data = ss.str();
    return writeFixSize(data.c_str(), data.size());
}

} // namespace muhui
} // namespace muhui
