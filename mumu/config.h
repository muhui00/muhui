/*****************************************
 * Copyright (C) 2022 * Ltd. All rights reserved.
 *
 * File name   : config.h
 * Author      : muhui
 * Created date: 2022-11-06 19:07:23
 * Description : 配置模块 
 *
 *******************************************/

//#pragma once
#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <yaml-cpp/yaml.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <functional>

#include "log.h"
#include "util.h"
#include "thread.h"

namespace muhui
{
//配置变量的基类
class ConfigVarBase
{
public:
    typedef std::shared_ptr<ConfigVarBase> ptr;
    ConfigVarBase(const std::string& name, const std::string& description = "")
        : m_name(name)
        , m_description(description) {
        std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
        }
    virtual ~ConfigVarBase() {}

    //获取配置参数的名称
    const std::string getName() const { return m_name; }
    //获取配置参数的描述
    const std::string getDescription() const { return m_description; }

    //转化成字符串
    virtual std::string toString() = 0;

    //从字符串初始化值
    virtual bool fromString(const std::string& val) = 0;

    //获取变量数据类型
    virtual std::string getTypeName() const = 0;
protected:
    ///配置参数的名称
    std::string m_name;
    ///配置参数的描述
    std::string m_description;
};
/**
 * @brief 类型转换模板类(F 源类型, T 目标类型)
 * 定义一个仿函数，必须重载()运算符
 */
template<class F, class T>
class LexcalCast
{
public:
    /**
     * @brief 类型转换
     * @param[in] v 源类型值
     * @return 返回v转换后的目标类型
     * @exception 当类型不可转换时抛出异常
     */
    T operator() (const F& v)
    {
        return boost::lexical_cast<T>(v);
    }

};

/**
 * @brief 类型转换模板类片特化(YAML String 转换成 std::vector<T>)
 */
template<class T>
class LexcalCast<std::string, std::vector<T>>
{
public:
    std::vector<T> operator()(const std::string& v)
    {
        /**
         * Loads the input string as a single YAML document.
         */
        YAML::Node node = YAML::Load(v);
        typename std::vector<T> vec;
        std::stringstream ss;
        for(size_t i = 0; i < node.size(); ++i)
        {
            //在对同一个stringstream对象重复赋值，就需要先对流使用clear()函数清空流的状态，
            //此时流占用的内存没有改变，会一直增加(stringstream不主动释放内存)，
            //若想改变内存(一般是清除内存，减少内存消耗)，需要再配合使用str("")清空stringstream的缓存。
            ss.str("");
            ss << node[i];
            vec.push_back(LexcalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};


/**
 * @brief 类型转换模板类片特化(std::vector<T> 转换成 YAML String)
 */
template<class T>
class LexcalCast<std::vector<T>, std::string>
{
public:
    std::string operator()(const std::vector<T>& vec)
    {
        YAML::Node node(YAML::NodeType::Sequence);
        for(auto& i : vec)
        {
            //sequence
            node.push_back(YAML::Load(LexcalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        //std::cout<< "vec:" << ss.str() << std::endl;
        return ss.str();
    }
};

/**
 * @brief 类型转换模板类片特化(YAML String 转换成 std::list<T>)
 */
template<class T>
class LexcalCast<std::string, std::list<T>>
{
public:
    std::list<T> operator()(const std::string& v)
    {
        /**
         * Loads the input string as a single YAML document.
         */
        YAML::Node node = YAML::Load(v);
        typename std::list<T> vec;
        std::stringstream ss;
        for(size_t i = 0; i < node.size(); ++i)
        {
            //在对同一个stringstream对象重复赋值，就需要先对流使用clear()函数清空流的状态，
            //此时流占用的内存没有改变，会一直增加(stringstream不主动释放内存)，
            //若想改变内存(一般是清除内存，减少内存消耗)，需要再配合使用str("")清空stringstream的缓存。
            ss.str("");
            ss << node[i];
            vec.push_back(LexcalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};


/**
 * @brief 类型转换模板类片特化(std::list<T> 转换成 YAML String)
 */
template<class T>
class LexcalCast<std::list<T>, std::string>
{
public:
    std::string operator()(const std::list<T>& vec)
    {
        YAML::Node node(YAML::NodeType::Sequence);
        for(auto& i : vec)
        {
            //sequence
            node.push_back(LexcalCast<T, std::string>()(i));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};


/**
 * @brief 类型转换模板类片特化(YAML String 转换成 std::set<T>)
 */
template<class T>
class LexcalCast<std::string, std::set<T>>
{
public:
    std::set<T> operator()(const std::string& v)
    {
        /**
         * Loads the input string as a single YAML document.
         */
        YAML::Node node = YAML::Load(v);
        typename std::set<T> s;
        std::stringstream ss;
        for(size_t i = 0; i < node.size(); ++i)
        {
            ss.str("");
            ss << node[i];
            s.insert(LexcalCast<std::string, T>()(ss.str()));
        }
        return s;
    }
};


/**
 * @brief 类型转换模板类片特化(std::set<T> 转换成 YAML String)
 */
template<class T>
class LexcalCast<std::set<T>, std::string>
{
public:
    std::string operator()(const std::set<T>& s)
    {
        YAML::Node node(YAML::NodeType::Sequence);
        for(auto& i : s)
        {
            //sequence
            node.push_back(LexcalCast<T, std::string>()(i));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};


/**
 * @brief 类型转换模板类片特化(YAML String 转换成 std::unordered_set<T>)
 */
template<class T>
class LexcalCast<std::string, std::unordered_set<T>>
{
public:
    std::unordered_set<T> operator()(const std::string& v)
    {
        /**
         * Loads the input string as a single YAML document.
         */
        YAML::Node node = YAML::Load(v);
        typename std::unordered_set<T> hash_set;
        std::stringstream ss;
        for(size_t i = 0; i < node.size(); ++i)
        {
            ss.str("");
            ss << node[i];
            hash_set.insert(LexcalCast<std::string, T>()(ss.str()));
        }
        return hash_set;
    }
};


/**
 * @brief 类型转换模板类片特化(std::unordered_set<T> 转换成 YAML String)
 */
template<class T>
class LexcalCast<std::unordered_set<T>, std::string>
{
public:
    std::string operator()(const std::unordered_set<T>& s)
    {
        YAML::Node node(YAML::NodeType::Sequence);
        for(auto& i : s)
        {
            //sequence
            node.push_back(LexcalCast<T, std::string>()(i));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/**
 * @brief 类型转换模板类片特化(YAML String 转换成 std::map<T>)
 */
template<class T>
class LexcalCast<std::string, std::map<std::string, T>>
{
public:
    std::map<std::string, T> operator()(const std::string& v)
    {
        /**
         * Loads the input string as a single YAML document.
         */
        YAML::Node node = YAML::Load(v);
        typename std::map<std::string ,T> map;
        std::stringstream ss;
        for(auto it = node.begin(); it != node.end(); ++it)
        {
            ss.str("");
            ss << it->second;
            map.insert(std::make_pair(it->first.Scalar(),
                                LexcalCast<std::string, T>()(ss.str())));
        }
        return map;
    }
};


/**
 * @brief 类型转换模板类片特化(std::map<T> 转换成 YAML String)
 */
template<class T>
class LexcalCast<std::map<std::string, T>, std::string>
{
public:
    std::string operator()(const std::map<std::string, T>& map)
    {
        YAML::Node node(YAML::NodeType::Sequence);
        for(auto& i : map)
        {
            //sequence
            node[i.first] = YAML::Load(LexcalCast<T, std::string>()(i.second));
            //std::stringstream ss;
            //ss << node;
            //std::cout<< "测试" << ss.str() << std::endl;
        }
        std::stringstream ss;
        ss << node;
        //std::cout << "map:" << ss.str() << std::endl;
        return ss.str();
    }
};


/**
 * @brief 类型转换模板类片特化(YAML String 转换成 std::map<T>)
 */
template<class T>
class LexcalCast<std::string, std::unordered_map<std::string, T>>
{
public:
    std::unordered_map<std::string, T> operator()(const std::string& v)
    {
        /**
         * Loads the input string as a single YAML document.
         */
        YAML::Node node = YAML::Load(v);
        typename std::unordered_map<std::string ,T> umap;
        std::stringstream ss;
        for(auto it = node.begin(); it != node.end(); ++it)
        {
            ss.str("");
            ss << it->second;
            umap.insert(std::make_pair(it->first.Scalar(),
                                LexcalCast<std::string, T>()(ss.str())));
        }
        return umap;
    }
};


/**
 * @brief 类型转换模板类片特化(std::unordered_map<T> 转换成 YAML String)
 */
template<class T>
class LexcalCast<std::unordered_map<std::string, T>, std::string>
{
public:
    std::string operator()(const std::unordered_map<std::string, T>& umap)
    {
        YAML::Node node(YAML::NodeType::Sequence);
        for(auto& i : umap)
        {
            //sequence
            node[i.first] = YAML::Load(LexcalCast<T, std::string>()(i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};


/**
 *  配置参数模板子类,保存对应类型的参数值
 *  T 参数的具体类型
 *          FromStr 从std::string转换成T类型的仿函数
 *          ToStr 从T转换成std::string的仿函数
 *          std::string 为YAML格式的字符串
 */
//偏特化（仿函数）模板
template<class T, class FromStr = LexcalCast<std::string, T>
                , class ToStr = LexcalCast<T, std::string>>
class ConfigVar : public ConfigVarBase
{
public:
    typedef RWMutex RWMutexType;
    typedef std::shared_ptr<ConfigVar> ptr;
    typedef std::function<void (const T& old_value, const T& new_value)> on_change_cb;
    /**
     * 通过参数名,参数值,描述构造ConfigVar
     * @param[in] name 参数名称有效字符为[0-9a-z_.]
     * @param[in] default_value 参数的默认值
     * @param[in] description 参数的描述
     */
    ConfigVar(const std::string& name
              , const T& default_value
              , const std::string& description)
        : ConfigVarBase(name, description)
        , m_val(default_value) {}

    /**
     * @brief 将参数值转换成YAML String
     * @exception 当转换失败抛出异常
     */
    std::string toString() override
    {
        try{
            //return boost::lexical_cast<std::string>(m_val);
            RWMutexType::ReadLock lock(m_mutex);
            return ToStr()(m_val);
        } catch (std::exception& e){
            MUHUI_LOG_ERROR(MUHUI_LOG_ROOT()) << "ConfigVar::toString exception"
                << e.what() << "convent:" << typeid(m_val).name() << "to string"; 
        }
        return "";
    }

    /**
     * @brief 从YAML String 转成参数的值
     * @exception 当转换失败抛出异常
     */
    bool fromString(const std::string& val) override
    {
        try{
            //m_val = boost::lexical_cast<T>(val);
            setValue(FromStr()(val));
        }catch(std::exception& e){
            MUHUI_LOG_ERROR(MUHUI_LOG_ROOT()) << "ConfigVar::fromString exception"
                << e.what() << "convert: string to" << typeid(m_val).name();
        }
        return false;
    }
    const T getValue() { 
        RWMutexType::ReadLock lock(m_mutex);
        return m_val; 
    }
    void setValue(const T& v) { 
        { //该{}的作用是添加一个局部域，当该段代码执行结束，ReadLock自动析构解锁，防止死锁
            RWMutexType::ReadLock lock(m_mutex);
            if(v == m_val){
                return;
            }
            for(auto& i : m_cbs){
                i.second(m_val, v);
            }
        }
        RWMutexType::WriteLock lock(m_mutex);
        m_val = v;
    }
    std::string getTypeName() const override { return typeid(T).name(); }
    //添加监听
    uint64_t addListener(on_change_cb cb){
        static uint64_t s_fun_id = 0;
        RWMutexType::WriteLock lock(m_mutex);
        ++ s_fun_id;
        m_cbs[s_fun_id] = cb;
        return s_fun_id;
    }
    //删除监听
    void delListener(uint64_t key){
        RWMutexType::WriteLock lock(m_mutex);
        m_cbs.erase(key);
    }

    //获取监听事件
    on_change_cb getListener(uint64_t key){
        RWMutexType::WriteLock lock(m_mutex);
        auto it = m_cbs.find(key);
        return it == m_cbs.end() ? nullptr : it->second;
    }

    //清空回调函数
    void clearListener()
    {
        RWMutexType::WriteLock lock(m_mutex);
        m_cbs.clear();
    }
private:
    T m_val;
    RWMutexType m_mutex;
    //变更回调函数组， uint64_t key, 要求唯一，一般可以用hash
    std::map<uint64_t, on_change_cb> m_cbs;
};

/**
 * @brief ConfigVar的管理类
 * @details 提供便捷的方法创建/访问ConfigVar
 */
class Config
{
public:
    typedef std::unordered_map<std::string, ConfigVarBase::ptr> ConfigVarMap;
    typedef RWMutex RWMutexType;
    template<class T>
    //typename 指出模板声明（或定义）中的非独立名称是类型名，而非变量名
    //Lookup是一个返回类型为ConfigVar<T>::ptr 的静态成员函数
    static typename ConfigVar<T>::ptr Lookup(const std::string& name,
                    const T& default_val, const std::string& description = "")
    {
        RWMutexType::WriteLock lock(GetMutex());
        //查找name是否在map内
        auto it = GetDatas().find(name);
        if(it != GetDatas().end())
        {
            //存在 查找是否存在T类型的ConfigVar指针对象
            //存在，记录信息，返回该指针对象
            //不存在，返回空指针，记录错误T类型信息
            //关于std::dynamic_pointer_cast<type>(e)的说明，e的类型必须与type相同，否侧返回空指针
            auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
            if(tmp)
            {
                //存在，记录信息，返回tmp
                MUHUI_LOG_INFO(MUHUI_LOG_ROOT()) << "Lookup name  = " << name << " exists";
                return tmp;
            } else {
                //不存在
                MUHUI_LOG_ERROR(MUHUI_LOG_ROOT()) << "Lookup name = " << name << " exists but type not:"
                    << typeid(T).name() << ", real_type = " << it->second->getTypeName()
                    << " " << it->second->toString();
                return nullptr;
            }
        }
        //不存在，先查询name是否存在非法字符
        if(name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789")
                != std::string::npos){
            //std::cout << "不存在" << std::endl;
            //name存在非法字符，记录错误信息，抛出异常
            MUHUI_LOG_ERROR(MUHUI_LOG_ROOT()) << "Lookup name invalid " << name;
            throw std::invalid_argument(name);
        }
        //新建ConfigVar对象指针，并将其加入到map中
        typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_val, description));
        GetDatas()[name] = v;
        return v;
    }

    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name)
    {
        RWMutexType::ReadLock lock(GetMutex());
        auto it = GetDatas().find(name);
        if(it == GetDatas().end())
        {
            return nullptr;
        }
        //将基类智能指针转化为子类智能指针返回
        return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
    }
    /**
     * @brief 查找配置参数,返回配置参数的基类
     * @param[in] name 配置参数名称
     */
    static ConfigVarBase::ptr LookupBase(const std::string& name);
    /**
     * @brief 使用YAML::Node初始化配置模块
     */
    static void LoadFromYaml(const YAML::Node& root);

    /**
     * @brief 遍历配置模块里面所有配置项
     * @param[in] cb 配置项回调函数
     */
    static void Visit(std::function<void(ConfigVarBase::ptr)> cb);

private:
    //这两个静态方法的作用是保证在程序运行的时候，
    //这两个静态变量可以保证最先初始化（在被调用的方法/对象之前），防止内存错误

    /**
     * @brief 返回所有的配置项
     *
     */
    static ConfigVarMap& GetDatas(){
        static ConfigVarMap s_datas;
        return s_datas;
    }
    /**
     * @brief 配置项的RWMutex
     */
    static RWMutexType& GetMutex() {
        static RWMutexType m_mutex;
        return m_mutex;
    }
};
}

#endif //__CONFIG_H__

