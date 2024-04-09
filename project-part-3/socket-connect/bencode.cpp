#include "bencode.h"

namespace Bencode {
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
}
