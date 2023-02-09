/**
 * @file servlet.cc
 * @author muhui (2571579302@qq.com)
 * @brief Servlet封装
 * @version 0.1
 * @date 2023-02-07
 */
#ifndef __MUHUI_HTTP_SERVLET_H
#define __MUHUI_HTTP_SERVLET_H
#include <memory>
#include <functional>
#include <unordered_map>
#include <string>
#include <utility>
#include <vector>

#include "http_session.h"
#include "http.h"
#include "mutex.h"
#include "util.h"

namespace muhui {
namespace http {
/**
 * @brief Servlet封装
 */
class Servlet {
public:
    typedef std::shared_ptr<Servlet> ptr;
    Servlet(std::string name) 
        : m_name(name) {}
    virtual ~Servlet() {}

    /**
     * @brief 处理请求
     * @param[in] request HTTP请求
     * @param[in] response HTTP响应
     * @param[in] session HTTP连接
     * @return int 
     */
    virtual int32_t handle(HttpRequest::ptr request
                        , HttpResponce::ptr response
                        , HttpSession::ptr session) = 0;
    const std::string& getName() const {return m_name;}
private:
    /// servlet名称
    std::string m_name;
};

/**
 * @brief FunctionServlet函数式
 */
class FunctionServlet : public Servlet{
public:
    typedef std::shared_ptr<FunctionServlet> ptr;

    /**
     * @brief 回调函数类型
     */
    typedef std::function<int32_t(HttpRequest::ptr request
                        , HttpResponce::ptr response
                        , HttpSession::ptr session)> callback;
    /**
     * @brief 构造函数
     */
    FunctionServlet(callback cb);
    virtual int32_t handle(HttpRequest::ptr request
                            , HttpResponce::ptr response
                            , HttpSession::ptr session) override;
private:
    //回调函数
    callback m_cb;
};

/**
 * @brief Servlet构造器（基类）
 */
class IServletCreator {
public:
    typedef std::shared_ptr<IServletCreator> ptr;
    virtual ~IServletCreator(){}
    //获取servlet
    virtual Servlet::ptr get() const = 0;
    //获取servlet名称
    virtual std::string getName() const = 0;
};

class HoldServletCreator : public IServletCreator {
public:
    typedef std::shared_ptr<HoldServletCreator> ptr;
    HoldServletCreator(Servlet::ptr slt)
        : m_servlet(slt) {}
    Servlet::ptr get() const override {
        return m_servlet;
    } 
    std::string getName() const override {
        return m_servlet->getName();
    }
private:
    Servlet::ptr m_servlet;
};
template <class T>
class ServletCreator : public IServletCreator {
public:
    typedef std::shared_ptr<ServletCreator> ptr;
    ServletCreator(){}
    Servlet::ptr get() const override {
        return Servlet::ptr(new T);
    }
    std::string getName() const override {
        return TypeToName<T>();
    }
};
/**
 * @brief Servlet分发器(管理类)
 */
class ServletDispatch : public Servlet {
public:
    typedef std::shared_ptr<ServletDispatch> ptr;
    typedef RWMutex RWMutexType;
    virtual int32_t handle(HttpRequest::ptr request
                            , HttpResponce::ptr response
                            , HttpSession::ptr session) override;
    /**
     * @brief 构造函数
     */
     ServletDispatch();

    /**
     * @brief 添加servlet
     * @param[in] uri 
     * @param[in] slt 
     */
    void addServlet(const std::string& uri, Servlet::ptr slt);
    /**
     * @brief 添加servlet
     * @param uir 
     * @param cb 回调函数
     */
    void addServlet(const std::string& uir, FunctionServlet::callback cb);

    /**
     * @brief 添加模糊匹配servlet
     * @param[in] uri uri 模糊匹配 /muhui_*
     * @param[in] slt 
     */
    void addGlobServlet(const std::string& uri, Servlet::ptr slt);
    void addGlobServlet(const std::string& uri, FunctionServlet::callback cb);
    void delServlet(const std::string& uri);
    void delGlobServlet(const std::string& uri);

    Servlet::ptr getDefault() const {return m_default;}

    void SetDefault(Servlet::ptr slt) {m_default = slt;}

    Servlet::ptr getServlet(const std::string& uri);
    /**
     * @brief 通过uri模糊匹配servlet
     * @param uri 
     * @return Servlet::ptr 
     */
    Servlet::ptr getGlobServlet(const std::string& uri);

    /**
     * @brief 通过uri获取servlet
     * @param uri 
     * @return Servlet::ptr 优先精准匹配，其次模糊匹配，最后返回默认 
     */
    Servlet::ptr getMatchedServlet(const std::string& uri);

    /**
     * @brief 添加servlet构造器
     * @param uri 
     * @param creator 
     */
    void addServletCreator(const std::string& uri, IServletCreator::ptr creator);
    void addGlobServletCreator(const std::string& uri, IServletCreator::ptr creator);
    template<class T> 
    void addServletCreator(const std::string& uri) {
        //std::make_shared 返回指定类型的智能指针
        addServletCreator(uri, std::make_shared<ServletCreator<T>>());
    }
    /**
     * @brief 所有构造器列表
     * 
     * @param infos 
     */
    void listAllServletCreator(std::map<std::string, IServletCreator::ptr>& infos);
    void listAllGlobServletCreator(std::map<std::string, IServletCreator::ptr>& infos);
private:
    RWMutexType m_mutex;
    /// 精准匹配servlet MAP
    /// uri(/muhui/xxx) ->servlet
    std::unordered_map<std::string, IServletCreator::ptr> m_datas;
    /// 模糊匹配servlet 数组
    /// uri(/muhui/*) ->servlet
    std::vector<std::pair<std::string, IServletCreator::ptr>> m_globs;
    /// 默认servlet ，所有路径都没有匹配到的时候使用
    Servlet::ptr m_default;
};

/**
 * @brief NOtFountServlet(默认返回404)
 */
class NotFountServlet : public Servlet{
public:
    typedef std::shared_ptr<NotFountServlet> ptr;
    NotFountServlet(const std::string& name);
    int32_t handle(HttpRequest::ptr request
                    , HttpResponce::ptr response
                    , HttpSession::ptr session) override;
private:
    std::string m_name;
    std::string m_content;
};
} // namespace http 
} // namespace muhui
#endif // !__MUHUI_HTTP_SERVLET_H
