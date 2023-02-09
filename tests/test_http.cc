#include "http/http.h"
#include "log.h"

void test_request() {
    muhui::http::HttpRequest::ptr req(new muhui::http::HttpRequest);
    req->setHeader("host", "www.sylar.top");
    req->setBody("hello muhui");

    req->dump(std::cout) << std::endl;
}

void test_responce() {
    muhui::http::HttpResponce::ptr rsp(new muhui::http::HttpResponce);
    rsp->setHeader("x-x", "muhui");
    rsp->setBody("hello muhui");
    rsp->setStatus((muhui::http::HttpStatus)400);
    rsp->setClose(true);
    rsp->dump(std::cout) << std::endl;
}

int main(int argc, char** argv) {
    test_request();
    test_responce();
    return 0;
}
