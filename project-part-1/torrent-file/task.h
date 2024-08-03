#pragma once

#include <string>
#include <vector>
#include <openssl/sha.h>
#include <fstream>
#include <variant>
#include <list>
#include <map>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <algorithm>
#include <string>
#include <chrono>


struct TorrentFile {
    std::string announce;//http://bttracker.debian.org:6969/announce
    std::string comment;//"Debian CD from cdimage.debian.org"
    std::vector<std::string> pieceHashes;
    size_t pieceLength;//262144
    size_t length;//406847488 - The length of the file, in bytes.
    std::string name;//debian-11.6.0-amd64-netinst.iso
    std::string infoHash;
};//pieces31040

using MapString = std::unordered_map<std::string, std::string>;
using VecString = std::vector<std::string>;
using VecVecString = std::vector<VecString>;
using DictType =  std::unordered_map<std::string, std::variant<std::string, MapString, VecString, VecVecString>>;
struct ForInfo{
    std::stringstream::pos_type pt_beg = -1;
    std::stringstream::pos_type pt_end = -1;
};

bool check(std::stringstream& content){
    if (content.peek() == 'e') {
        content.get();
        return false;
    }
    return true;
}

long GetNumber(std::string str){
    long res = 0, dig = 1;
    for(size_t i = str.size(); i>0; --i){
        res += dig * (str[i - 1] - '0');
        dig *= 10;
    }
    return res;
}

void GetKey(std::stringstream& content, std::string& key){
    long cnt, size_cnt;
    std::string array(100, '0');

    size_cnt = content.getline(&array[0], (std::streamsize)array.size(), ':').gcount() - 1; ///number

    cnt = GetNumber(std::string(array.begin(), array.begin() + size_cnt));
    char elem;
    for(size_t i = 0; i < cnt; ++i){
        content.get(elem);
        key += elem;
    }
}

void ExtractNumber(std::stringstream& content, std::string& number) {
    char elem = 'i';
    content.get();
    while (elem != 'e') {
        content.get(elem);
        if (elem != 'e')
            number += elem;
    }
}

void ExtractLst(std::stringstream& content,VecString& lst){
    std::string buffer;

    while (!content.eof()) {
        if (!check(content)) {
            return;
        }
        GetKey(content, buffer);
        lst.push_back(buffer);
    }
}

void ExtractLstLst(std::stringstream& content, std::string& name, VecVecString& lstlst){
    size_t i = 0;
    std::string stroka;
    while(!content.eof()){
        if(!check(content)){
            return;
        }
        if(content.peek() == 'l'){
            content.get();
            lstlst.push_back(std::move(VecString ()));
            ExtractLst(content, lstlst[i]);
            ++i;
        }
    }
}

void RecursiveParsing(std::stringstream& content, DictType& dictionary, const std::string& state, MapString& mp, ForInfo& fi){
    char start;
    content >> start;
    while(true) {
        std::string key, buffer;
        if (content.peek() == 'e') {
            if (fi.pt_beg != -1 && fi.pt_end == -1) {
                fi.pt_end = content.tellg();
            }
            content.get();
            break;
        }
        GetKey(content, key);
        if (key == "info") {///extract line
            fi.pt_beg = content.tellg();
        }

        if (content.peek() == 'd') {///another map has begun, info
            RecursiveParsing(content, dictionary, "SECOND_MAP", mp, fi);
            dictionary.insert({std::move(key), std::move(mp)});

        } else if (content.peek() == 'l') {///list has begun
            content.get();
            if (content.peek() == 'l') {///second list
                VecVecString lstlst;
                ExtractLstLst(content, key, lstlst);
                dictionary.insert({key, std::move(lstlst)});
            } else {
                VecString lst;
                ExtractLst(content, lst);
                dictionary.insert({key, std::move(lst)});
            }
        } else if (content.peek() == 'i') {
            ExtractNumber(content, buffer);
        } else {///number
            GetKey(content, buffer);
        }

        if (state == "SECOND_MAP") {
            mp.insert({std::move(key), std::move(buffer)});
        } else {
            dictionary[key] = buffer;
        }

    }
}

/*
 * Функция парсит .torrent файл и загружает информацию из него в структуру `TorrentFile`. Как устроен .torrent файл, можно
 * почитать в открытых источниках (например http://www.bittorrent.org/beps/bep_0003.html).
 * После парсинга файла нужно также заполнить поле `infoHash`, которое не хранится в файле в явном виде и должно быть
 * вычислено. Алгоритм вычисления этого поля можно найти в открытых источника, как правило, там же,
 * где описание формата .torrent файлов.
 * Данные из файла и infoHash будут использованы для запроса пиров у торрент-трекера. Если структура `TorrentFile`
 * была заполнена правильно, то трекер найдет нужную раздачу в своей базе и ответит списком пиров. Если данные неверны,
 * то сервер ответит ошибкой.
 */
TorrentFile LoadTorrentFile(const std::string& filename) {
    //auto start = std::chrono::system_clock::now()
    TorrentFile tr;
    std::ifstream cin(filename, std::ios::binary);
    std::stringstream content;
    content << cin.rdbuf();
    DictType dictionary;


    std::string info;
    MapString mp;

    ForInfo fi;
    //RecursiveParsing(std::stringstream& content, DictType& dictionary, const std::string& state, ForMapString& mp, ForVecString& lst, ForVecVecString& lstLst, ForInfo& fi){
    RecursiveParsing(content, dictionary, "FIRST_MAP", mp, fi);

    content.seekg(fi.pt_beg);
    char element;
    for(auto i = fi.pt_beg; i <= fi.pt_end; i+=1){
        content.get(element);
        info += element;
    }

    ////////////////


    tr.announce = std::move(std::get<std::string>(dictionary["announce"]));

    tr.comment = std::move(std::get<std::string>(dictionary["comment"]));

    tr.length = GetNumber(std::get<MapString>(dictionary["info"]).find("length")->second);

    tr.name = std::move(std::get<MapString>(dictionary["info"]).find("name")->second);

    tr.pieceLength = GetNumber(std::get<MapString>(dictionary["info"]).find("piece length")->second);

    std::string pieces_ = std::move(std::get<MapString>(dictionary["info"]).find("pieces")->second);

    std::stringstream hash;
    long step = 20;
    char elem;
    hash << pieces_;
    std::string temp;

    while(!hash.eof()) {
        for (long i = 0; i < step; ++i) {
            if (hash.peek() == hash.eof()) {
                hash.get();
                break;
            }
            hash.get(elem);
            temp += elem;
        }
        tr.pieceHashes.push_back(std::move(temp));
        temp.clear();
    }
    unsigned char sha[20];
    SHA1((unsigned char *) info.c_str(), info.size(), sha);
    for(unsigned char i : sha)
        tr.infoHash += (char)i;
    return tr;
}

