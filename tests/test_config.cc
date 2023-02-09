/*****************************************
 * Copyright (C) 2022 * Ltd. All rights reserved.
 *
 * File name   : test_config.cc
 * Author      : muhui
 * Created date: 2022-11-06 22:17:34
 * Description : 
 *
 *******************************************/

#define LOG_TAG "TEST_CONFIG"
#include "config.h"
#include "log.h"
#include <yaml-cpp/yaml.h>
#include <iostream>
muhui::ConfigVar<int>::ptr g_int_value_config = 
    muhui::Config::Lookup("system.port", (int)8080, "system port");
#if 0
muhui::ConfigVar<float>::ptr g_int_valuex_config = 
    muhui::Config::Lookup("system.port", (float)8080, "system port");
#endif

muhui::ConfigVar<float>::ptr g_float_value_config = 
    muhui::Config::Lookup("system.value", (float)10.2f, "system port");

muhui::ConfigVar<std::vector<int>>::ptr g_int_vec_value_config = 
    muhui::Config::Lookup("system.int_vec", std::vector<int>{1, 2}, "system int vec");

muhui::ConfigVar<std::list<int>>::ptr g_int_list_value_config = 
    muhui::Config::Lookup("system.int_list", std::list<int>{1, 2}, "system int list");

muhui::ConfigVar<std::set<int>>::ptr g_int_set_value_config = 
    muhui::Config::Lookup("system.int_set", std::set<int>{1, 2}, "system int set");

muhui::ConfigVar<std::unordered_set<int>>::ptr g_int_uset_value_config = 
    muhui::Config::Lookup("system.int_uset", std::unordered_set<int>{1, 2}, "system int uset");

muhui::ConfigVar<std::map<std::string, int>>::ptr g_str_int_map_value_config = 
    muhui::Config::Lookup("system.str_int_map", std::map<std::string, int>{{"k", 2}}, "system str int map");

muhui::ConfigVar<std::unordered_map<std::string, int>>::ptr g_str_int_umap_value_config = 
    muhui::Config::Lookup("system.str_int_umap", std::unordered_map<std::string, int>{{"k", 2}}, "system str int umap");


//遍历解析后的数据
void print_yaml(const YAML::Node& node, int level)
{
    //遍历标量 单个的、不可再分的值
    if(node.IsScalar()){
        MUHUI_LOG_INFO(MUHUI_LOG_ROOT()) <<" node it scalar " << "-->"<< std::string(level * 4 , ' ')
            << node.Scalar() << " - " << node.Type() << " - " << level;
    } else if (node.IsNull()){
         MUHUI_LOG_INFO(MUHUI_LOG_ROOT()) << std::string(level * 4 , ' ')
            << " NULL -  " << node.Type() << " - " << level;
    } else if (node.IsMap()){ //对象（字典）
        for(auto it = node.begin(); it != node.end(); it ++)
        {
            //std::cout << "it->first : " << it->first << "  it->second.Type() : "  
             //   << it->second.Type() << std::endl;
            MUHUI_LOG_INFO(MUHUI_LOG_ROOT()) << " node it map " << "-->" << std::string(level * 4, ' ')
                << it->first << " - " << it->second.Type() << " - " << level;
            print_yaml(it->second, level + 1);
        }
        //序列 (数组)
    } else if (node.IsSequence()){
        for(size_t i = 0; i < node.size(); ++i)
        {
            MUHUI_LOG_INFO(MUHUI_LOG_ROOT()) << " node it sequence " << "-->" << std::string(level * 4, ' ')
                << i << " - " << node[i].Type() << " - " << level;
            print_yaml(node[i], level + 1);

        }
    }
}

void test_yaml()
{
    YAML::Node root = YAML::LoadFile("/home/muhui/mumu/bin/conf/test.yml");

    //MUHUI_LOG_INFO(MUHUI_LOG_ROOT()) << root;
    //std::cout << root << std::endl;
    print_yaml(root, 0);
}

void test_config()
{
    MUHUI_LOG_INFO(MUHUI_LOG_ROOT()) << "before: " << g_int_value_config->getValue();
    MUHUI_LOG_INFO(MUHUI_LOG_ROOT()) << "before: " << g_float_value_config->toString();

#define XX(g_var, name, prefix) \
    { \
        auto& v= g_var->getValue(); \
        for(auto& i : v) \
        { \
            MUHUI_LOG_INFO(MUHUI_LOG_ROOT()) << #prefix "  " #name ": " << i; \
        } \
            MUHUI_LOG_INFO(MUHUI_LOG_ROOT()) << #prefix "  " #name " yaml: " << g_var->toString(); \
    }
#define XX_M(g_var, name, prefix) \
    { \
        auto& v= g_var->getValue(); \
        for(auto& i : v) \
        { \
            MUHUI_LOG_INFO(MUHUI_LOG_ROOT()) << #prefix "  " #name ": {"  \
                    << i.first << " - " << i.second << "}"; \
        } \
            MUHUI_LOG_INFO(MUHUI_LOG_ROOT()) << #prefix "  " #name " yaml: " << g_var->toString(); \
    }

    XX(g_int_vec_value_config, int_vec, before);
    XX(g_int_list_value_config, int_list, before);
    XX(g_int_set_value_config, int_set, before);
    XX(g_int_uset_value_config, int_uset, before);
    XX_M(g_str_int_map_value_config, str_int_map, before);
    XX_M(g_str_int_umap_value_config, str_int_umap, before);
    YAML::Node root = YAML::LoadFile("/home/muhui/mumu/bin/conf/test.yml");
    muhui::Config::LoadFromYaml(root);

    MUHUI_LOG_INFO(MUHUI_LOG_ROOT()) << g_int_value_config->getValue();
    MUHUI_LOG_INFO(MUHUI_LOG_ROOT()) << g_float_value_config->toString();

    XX(g_int_vec_value_config, int_vec, after);
    XX(g_int_list_value_config, int_list, after);
    XX(g_int_set_value_config, int_set, after);
    XX(g_int_uset_value_config, int_uset, after);
    XX_M(g_str_int_map_value_config, str_int_map, after);
    XX_M(g_str_int_umap_value_config, str_int_umap, after);
   }

class Person
{
public:
    Person(){}
    std::string m_name = "a";
    int m_age = 0;
    bool m_sex = 0;

    std::string toString() const
    {
        std::stringstream ss;
        ss << "[Person name = " << m_name
            << " age = " << m_age
            << " sex = " << m_sex
            << "]";
        return ss.str();
    }
    //重载==运算符
    bool operator==(const Person& other) const{
        return m_name == other.m_name 
            && m_age == other.m_age
            && m_sex == other.m_sex;
    }
};

namespace muhui
{

/**
 * @brief 类型转换模板类片特化(YAML String 转换成 Person)
 */
template<>
class LexcalCast<std::string, Person>
{
public:
    Person operator()(const std::string& v)
    {
        /**
         * Loads the input string as a single YAML document.
         */
        YAML::Node node = YAML::Load(v);
        Person p;
        p.m_name = node["name"].as<std::string>();
        p.m_age = node["age"].as<int>();
        p.m_sex = node["sex"].as<bool>();
        return p;
    }
};


/**
 * @brief 类型转换模板类片特化(p 转换成 YAML String)
 */
template<>
class LexcalCast<Person, std::string>
{
public:
    std::string operator()(const Person& p)
    {
        YAML::Node node;
        node["name"] = p.m_name;
        node["age"] = p.m_age;
        node["sex"] = p.m_sex;
        std::stringstream ss;
        ss << node;
        //std::cout << "Person:" << ss.str() << std::endl;
        return ss.str();
    }
};
}

muhui::ConfigVar<Person>::ptr g_person = 
    muhui::Config::Lookup("class.person", Person(), "system person");

muhui::ConfigVar<std::map<std::string, Person>>::ptr g_person_map = 
    muhui::Config::Lookup("class.map", std::map<std::string, Person>{}, "system person");

muhui::ConfigVar<std::map<std::string, std::vector<Person>>>::ptr g_person_vec_map = 
    muhui::Config::Lookup("class.vec_map", std::map<std::string, std::vector<Person>>{}, "system person");


void test_class()
{
    MUHUI_LOG_INFO(MUHUI_LOG_ROOT()) << "before: " << g_person->getValue().toString()
        << " - " << g_person->toString();
#define XX_PM(g_var, prefix) \
    { \
        auto m = g_person_map->getValue(); \
        for(auto& i : m) \
        { \
            MUHUI_LOG_INFO(MUHUI_LOG_ROOT()) << prefix << ": " << i.first << " - " \
                << i.second.toString(); \
        } \
        MUHUI_LOG_INFO(MUHUI_LOG_ROOT()) << prefix << ": size = " << m.size(); \
    }
    //添加一个监听回调事件
    g_person->addListener([](const Person& old_value, const Person& new_value){
        MUHUI_LOG_INFO(MUHUI_LOG_ROOT()) << "old_value = " << old_value.toString()
            << "   new_value = " << new_value.toString();
    });
    //XX_PM(g_person_map, "cllass.map before");
    //XX_PM(g_person_vec_map, "class.vec_map before");
    //MUHUI_LOG_INFO(MUHUI_LOG_ROOT()) << "class.vec_map before:" << g_person_vec_map->toString();
    YAML::Node root = YAML::LoadFile("/home/muhui/mumu/bin/conf/test.yml");
    //MUHUI_LOG_INFO(MUHUI_LOG_ROOT()) << root;
    muhui::Config::LoadFromYaml(root);
    MUHUI_LOG_INFO(MUHUI_LOG_ROOT()) << "after: " << g_person->getValue().toString()
         << " - " << g_person->toString(); 
    //XX_PM(g_person_map, "calss.map after");
    //XX_PM(g_person_vec_map, "class.vec_map after");
    //MUHUI_LOG_INFO(MUHUI_LOG_ROOT()) << "class.vec_map after:" << g_person_vec_map->toString();
}

void test_log()
{
    static muhui::Logger::ptr system_log = MUHUI_LOG_NAME("system");
    MUHUI_LOG_INFO(system_log) << "hello system" << std::endl;
    std::cout << muhui::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    YAML::Node root = YAML::LoadFile("/home/muhui/mumu/bin/conf/log.yml");
    //std::cout << "----------------" << std::endl;
    //std::cout << root << std::endl;
    muhui::Config::LoadFromYaml(root);
    std::cout << "======================" << std::endl;
    //system_log->addAppender(muhui::LogAppender::ptr(new muhui::StdoutLogAppender));
    std::cout << muhui::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    MUHUI_LOG_INFO(system_log) << "hello system" << std::endl;
    system_log->setFormatter("%d - %m%n");
    MUHUI_LOG_INFO(system_log) << "hello system" << std::endl;
}
int main(int argc, char *argv[]) {
    //test_yaml();
    //std::cout << "---------------------------------------------" << std::endl;
    //test_config();
    //test_class();
    test_log();
    muhui::Config::Visit([](muhui::ConfigVarBase::ptr var){
        MUHUI_LOG_INFO(MUHUI_LOG_ROOT()) << "name = " << var->getName()
                            << " description = " << var->getDescription()
                            << " typename = " << var->getTypeName()
                            << " value = " << var->toString();
    });
    return 0;
}

