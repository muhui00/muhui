#include "address.h"
#include "http/http_server.h"
#include "iomanager.h"

void run() {
    muhui::http::HttpServer::ptr server(new muhui::http::HttpServer);
    muhui::Address::ptr addr = muhui::Address::LookupAny("0.0.0.0:8020");
    while(!server->bind(addr)) {
        sleep(2);
    } 
    auto sd = server->getServletDispatch();
    sd->addServlet("/mumu/xx", [](muhui::http::HttpRequest::ptr req,
                                  muhui::http::HttpResponce::ptr rsp,
                                  muhui::http::HttpSession::ptr session){
        std::cout << "addServlet test" << std::endl;
        rsp->setBody(req->toString());
        return 0;
    });
    sd->addGlobServlet("/mumu/*", [](muhui::http::HttpRequest::ptr req,
                                  muhui::http::HttpResponce::ptr rsp,
                                  muhui::http::HttpSession::ptr session){
        rsp->setBody("Glob:\r\n" + req->toString());
        return 0;
    });
    server->start();
}

int main(int argc, char** argv) {
    muhui::IOManager iom(2);
    iom.schedule(run);    
    return 0;
}
