#pragma once

#include "peer.h"
#include "torrent_file.h"
#include <string>
#include <vector>
#include <openssl/sha.h>
#include <fstream>
#include <variant>
#include <list>
#include <map>
#include <sstream>
#include <cpr/cpr.h>
#include <unordered_map>
#include <iostream>

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

class TorrentTracker {
public:
    /*
     * url - адрес трекера, берется из поля announce в .torrent-файле
     */
    TorrentTracker(const std::string& url) : url_(std::move(url)){}

    /*
     * Получить список пиров у трекера и сохранить его для дальнейшей работы.
     * Запрос пиров происходит посредством HTTP GET запроса, данные передаются в формате bencode.
     * Такой же формат использовался в .torrent файле.
     * Подсказка: посмотрите, что было написано в main.cpp в домашнем задании torrent-file
     *
     * tf: структура с разобранными данными из .torrent файла из предыдущего домашнего задания.
     * peerId: id, под которым представляется наш клиент.
     * port: порт, на котором наш клиент будет слушать входящие соединения (пока что мы не слушаем и на этот порт никто
     *  не сможет подключиться).
     */
    void UpdatePeers(const TorrentFile& tf, std::string peerId, int port){
        cpr::Response res = cpr::Get(
                cpr::Url{tf.announce},
                cpr::Parameters {
                        {"info_hash", tf.infoHash},
                        {"peer_id", peerId},
                        {"port", std::to_string(port)},
                        {"uploaded", std::to_string(0)},
                        {"downloaded", std::to_string(0)},
                        {"left", std::to_string(tf.length)},
                        {"compact", std::to_string(1)}
                },
                cpr::Timeout{20000}
        );

        std::stringstream content;
        content << res.text;
        //std::cout << res.text << std::endl;
        //RecursiveParsing(std::stringstream& content, DictType& dictionary, const std::string& state, ForMapString& mp, ForVecString& lst, ForVecVecString& lstLst, ForInfo& fi){
        DictType dictionary;



        std::string info;
        MapString mp;
        ForInfo fi;
        RecursiveParsing(content, dictionary, "FIRST_MAP", mp,  fi);

        std::stringstream peers_str;
        peers_str << std::get<std::string>(dictionary["peers"]);
        for(size_t i = 0; i < std::get<std::string>(dictionary["peers"]).size(); i+=6){
            std::string ip_;
            for (size_t j = 0; j < 4; ++j) {
                ip_ += std::to_string(((int) peers_str.get()));
                if (j < 3)
                    ip_ += ".";
            }
            auto port_ = static_cast<uint16_t>((int)  peers_str.get() << 8) | static_cast<uint16_t>((int)  peers_str.get());
            peers_.push_back({ip_, port_});
        }
    }

    /*
     * Отдает полученный ранее список пиров
     */
    const std::vector<Peer>& GetPeers() const{
        return peers_;
    }

private:
    std::string url_;
    std::vector<Peer> peers_;
};


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

