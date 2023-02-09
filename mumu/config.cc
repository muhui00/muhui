/*****************************************
 * Copyright (C) 2022 * Ltd. All rights reserved.
 *
 * File name   : config.cc
 * Author      : muhui
 * Created date: 2022-11-06 19:55:01
 * Description : 
 *
 *******************************************/

#define LOG_TAG "CONFIG"
#include "config.h"

namespace muhui
{
ConfigVarBase::ptr Config::LookupBase(const std::string& name)
{
    RWMutexType::WriteLock lock(GetMutex());
    auto it = GetDatas().find(name);
    return it == GetDatas().end() ? nullptr : it->second;
}
//"A.B, 10"
//-->
//A:
//  B: 10
//  C: str
static void ListAllMember(const std::string& prefix,
                          const YAML::Node& node,
                          std::list<std::pair<std::string, YAML::Node>>& output)
{
    if(prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789")
       != std::string::npos){
        MUHUI_LOG_ERROR(MUHUI_LOG_ROOT()) << "Config invalid name " << prefix << " " << node;
        return;
    }
    //将拼接后的字符串 及对应的node放入到map
    output.emplace_back(std::make_pair(prefix, node));
    //std::cout << "[prefix]" << prefix << std::endl;
    //std::cout << "node.Type():" << node.Type() << "-->[node]" << node  << std::endl;
    //std::cout << "**************" << std::endl;
    //遍历字典，并拼接字符串 A.B形式
    if(node.IsMap())
    {
        for(auto it = node.begin();
            it != node.end(); ++ it)
        {
            //std::cout << "it->first-" << it->first << std::endl; 
            //std::cout << "it->second-" << it->second << std::endl; 
            //std::cout << "it->first.Scalar() - " << it->first.Scalar() << "     prefix - " << prefix << std::endl;
            //std::string str = prefix + "." + it->first.Scalar();
            //std::cout << "str : " << str << std::endl;
            ListAllMember(prefix.empty() ? it->first.Scalar() 
                          : prefix + "." + it->first.Scalar(), it->second, output);
        }
    }
}
void Config::LoadFromYaml(const YAML::Node& root)
{
    std::list<std::pair<std::string, YAML::Node>> all_nodes;
    ListAllMember("", root, all_nodes);
    for(auto& i : all_nodes)
    {
        //std::cout << i.first << std::endl;
        //std::cout << i.second << std::endl;
        std::string key = i.first;
        if(key.empty())
        {
            continue;
        }
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        ConfigVarBase::ptr var = LookupBase(key);

        if(var)
        {
            if(i.second.IsScalar())
            {
                var->fromString(i.second.Scalar());
            } else {
                std::stringstream ss;
                ss << i.second;
                var->fromString(ss.str());
            }
        }
    }
}

void Config::Visit(std::function<void(ConfigVarBase::ptr)> cb)
{
    RWMutexType::ReadLock lock(GetMutex());
    ConfigVarMap& m = GetDatas();
    for(auto it = m.begin(); it != m.end(); ++it) {
        cb(it->second);
    }
}

}
