#include "byte_tools.h"
#include "piece.h"
#include <iostream>
namespace {
    constexpr size_t BLOCK_SIZE = 1 << 14;
}
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

bool Piece::HashMatches() const{
    return this->GetDataHash() == this->GetHash();
}

Block* Piece::FirstMissingBlock(){
    for(auto& block : blocks_){
        if(block.status == Block::Status::Missing){
            return &block;
        }
    }
    throw std::runtime_error("SOMETHING WENT WRONG IN Block* Piece::FirstMissingBlock()");
}

size_t Piece::GetIndex() const{
    return index_;
}

void Piece::SaveBlock(size_t blockOffset, std::string data){
    for(auto& block : blocks_){
        if(block.offset == blockOffset){
            block.data = std::move(data);
            block.status = Block::Status::Retrieved;
            break;
        }
    }
}

bool Piece::AllBlocksRetrieved() const{
    int i = 0;
    for(auto& block : blocks_){
        if(block.status != Block::Status::Retrieved){
            return false;
        }
        ++i;
    }
    return true;
}

std::string Piece::GetData() const{
    std::string allData;
    for(auto& block : blocks_){
        allData += block.data;
    }
    return allData;
}

std::string Piece::GetDataHash() const{
    std::string allData = this->GetData();
    return CalculateSHA1(allData);
}

const std::string& Piece::GetHash() const{
    return hash_;
}

void Piece::Reset(){
    for(auto& block : blocks_){
        block.data.clear();
        block.status = Block::Status::Missing;
    }
}
