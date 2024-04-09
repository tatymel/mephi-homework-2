#include "byte_tools.h"
#include "piece.h"
#include <iostream>
#include <algorithm>
/*private:
const size_t index_, length_;
const std::string hash_;
std::vector<Block> blocks_;

 struct Block {

    enum Status {
        Missing = 0,
        Pending,
        Retrieved,
    };

    uint32_t piece;  // id части файла, к которой относится данный блок
    uint32_t offset;  // смещение начала блока относительно начала части файла в байтах
    uint32_t length;  // длина блока в байтах
    Status status;  // статус загрузки данного блока
    std::string data;  // бинарные данные
};
};*/
namespace {
constexpr size_t BLOCK_SIZE = 1 << 14;
}
/*
  * index -- номер части файла, нумерация начинается с 0
  * length -- длина части файла. Все части, кроме последней, имеют длину, равную `torrentFile.pieceLength`; piece length = 262144
  * hash -- хеш-сумма части файла, взятая из `torrentFile.pieceHashes`
  */
Piece::Piece(size_t index, size_t length, std::string hash) :
        index_(index),
        length_(length),
        hash_(std::move(hash))
{
    uint32_t length_last_block = length_ % BLOCK_SIZE;
    for (uint32_t i = 0; i < length_ / BLOCK_SIZE; ++i) {
        blocks_.push_back({(uint32_t)index_, i * (uint32_t) BLOCK_SIZE, BLOCK_SIZE, Block::Status::Missing, ""});
    }
    if(length_last_block > 0){
        uint32_t piece = blocks_.size();
        blocks_.push_back({(uint32_t)index_, piece * (uint32_t) BLOCK_SIZE, length_last_block, Block::Status::Missing, ""});
    }
}

    /*
    * Совпадает ли хеш скачанных данных с ожидаемым
    */
    bool Piece::HashMatches() const{
        return this->GetDataHash() == this->GetHash();
    }

    /*
     * Дать указатель на отсутствующий (еще не скачанный и не запрошенный) блок
     */
    Block* Piece::FirstMissingBlock(){
        for(auto& block : blocks_){
            if(block.status == Block::Status::Missing){
                return &block;
            }
        }
        std::cout << "SOMETHING WENT WRONG IN Block* Piece::FirstMissingBlock()" << std::endl;
    }

    /*
     * Получить порядковый номер части файла
     */
    size_t Piece::GetIndex() const{
        return index_;
    }

    /*
     * Сохранить скачанные данные для какого-то блока
     */
    void Piece::SaveBlock(size_t blockOffset, std::string data){
        for(auto& block : blocks_){
            if(block.offset == blockOffset){
                block.data = std::move(data);
                block.status = Block::Status::Retrieved;
                break;
            }
        }
    }

    /*
     * Скачали ли уже все блоки
     */
    bool Piece::AllBlocksRetrieved() const{
        int i = 0;
        for(auto& block : blocks_){
            if(block.status != Block::Status::Retrieved){
                std::cout << "block: " << i << " != Block::Status::Retrieved" << std::endl;
                return false;
            }
            ++i;
        }
        return true;
    }

    /*
     * Получить скачанные данные для части файла
     */
    std::string Piece::GetData() const{
        std::string allData;
        for(auto& block : blocks_){
            allData += block.data;
        }
        return allData;
    }

    /*
     * Посчитать хеш по скачанным данным
     */
    std::string Piece::GetDataHash() const{
        std::string allData = this->GetData();
        return CalculateSHA1(allData);
    }

    /*
     * Получить хеш для части из .torrent файла
     */
    const std::string& Piece::GetHash() const{
        return hash_;
    }

    /*
     * Удалить все скачанные данные и отметить все блоки как Missing
     */
    void Piece::Reset(){
        for(auto& block : blocks_){
            block.data.clear();
            block.status = Block::Status::Missing;
        }
    }
