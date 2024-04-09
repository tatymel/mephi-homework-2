#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <variant>
#include <list>
#include <map>
#include <sstream>
#include <map>
#include <iostream>
#include <unordered_map>
#include <algorithm>
#include <chrono>

namespace Bencode {
    using MapString = std::unordered_map<std::string, std::string>;
    using VecString = std::vector<std::string>;
    using VecVecString = std::vector<VecString>;
    using DictType =  std::unordered_map<std::string, std::variant<std::string, MapString, VecString, VecVecString>>;
    struct ForInfo{
        std::stringstream::pos_type pt_beg = -1;
        std::stringstream::pos_type pt_end = -1;
    };
    bool check(std::stringstream& content);
    long long GetNumber(std::string str);
    void GetKey(std::stringstream& content, std::string& key);
    void ExtractNumber(std::stringstream& content, std::string& number);
    void ExtractLst(std::stringstream& content,VecString& lst);
    void ExtractLstLst(std::stringstream& content, std::string& name, VecVecString& lstlst);
    void RecursiveParsing(std::stringstream& content, DictType& dictionary, const std::string& state, MapString& mp, ForInfo& fi);
/*
 * В это пространство имен рекомендуется вынести функции для работы с данными в формате bencode.
 * Этот формат используется в .torrent файлах и в протоколе общения с трекером
 */

}
