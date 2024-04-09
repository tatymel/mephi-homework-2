#include "torrent_file.h"
#include "bencode.h"
#include "byte_tools.h"
#include <openssl/sha.h>
#include <variant>
#include <sstream>

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

    Bencode::DictType dictionary;
    Bencode::MapString mp;
    Bencode::ForInfo fi;
    std::string info;

    RecursiveParsing(content, dictionary, "FIRST_MAP", mp, fi);

    content.seekg(fi.pt_beg);
    char element;
    for(auto i = fi.pt_beg; i <= fi.pt_end; i+=1){
        content.get(element);
        info += element;
    }

    tr.announce = std::move(std::get<std::string>(dictionary["announce"]));

    tr.comment = std::move(std::get<std::string>(dictionary["comment"]));

    tr.length = Bencode::GetNumber(std::get<Bencode::MapString>(dictionary["info"]).find("length")->second);

    tr.name = std::move(std::get<Bencode::MapString>(dictionary["info"]).find("name")->second);

    tr.pieceLength = Bencode::GetNumber(std::get<Bencode::MapString>(dictionary["info"]).find("piece length")->second);
    std::string pieces_ = std::move(std::get<Bencode::MapString>(dictionary["info"]).find("pieces")->second);

    for(size_t i = 0; i < pieces_.length(); i+=20){
        tr.pieceHashes.push_back(pieces_.substr(i, 20));
    }

    tr.infoHash = CalculateSHA1(info);
    return tr;
}


