#include "piece_storage.h"
#include <iostream>
/*
* Хранилище информации о частях скачиваемого файла.
* В этом классе отслеживается информация о том, какие части файла осталось скачать
*/

PieceStorage::PieceStorage(const TorrentFile& tf, const std::filesystem::path& outputDirectory, size_t PiecesToDownload)
                : piecesInProgressCount_(0),
                  pieceLength_(tf.pieceLength),
                  piecesCount_(tf.pieceHashes.size()),
                  ourFile_(outputDirectory / tf.name, std::ios::binary | std::ios::out)
{
    std::lock_guard lock(mutex_);
    for(size_t i = 0; i < PiecesToDownload; ++i){
        remainPieces_.push(std::make_shared<Piece>(i, tf.pieceLength, tf.pieceHashes[i]));
    }
    std::filesystem::resize_file(outputDirectory / tf.name, tf.length);
}

PiecePtr PieceStorage::GetNextPieceToDownload() {
    std::lock_guard lock(mutex_);
    if(remainPieces_.empty())
        return nullptr;
    PiecePtr temp = remainPieces_.front();
    remainPieces_.pop();
    ++piecesInProgressCount_;
    return temp;
}

void PieceStorage::AddPieceToQueue(const PiecePtr& piece){ //exception occurred
    std::lock_guard lock(mutex_);
    --piecesInProgressCount_;
    remainPieces_.push(piece);
}

void PieceStorage::PieceProcessed(const PiecePtr& piece) {
    std::lock_guard lock(mutex_);
    SavePieceToDisk(piece);
}

bool PieceStorage::QueueIsEmpty() const {
    std::lock_guard lock(mutex_);
    return remainPieces_.empty();
}

size_t PieceStorage::TotalPiecesCount() const {
    std::lock_guard lock(mutex_);
    return piecesCount_;
}


size_t PieceStorage::PiecesSavedToDiscCount() const{
    std::lock_guard lock(mutex_);
    return piecesSavedToDiscIndices_.size();
}

void PieceStorage::CloseOutputFile(){
    std::lock_guard lock(mutex_);
    ourFile_.close();
}

const std::vector<size_t>& PieceStorage::GetPiecesSavedToDiscIndices() const{
    std::lock_guard lock(mutex_);
    return piecesSavedToDiscIndices_;
}

size_t PieceStorage::PiecesInProgressCount() const{
    std::lock_guard lock(mutex_);
    return piecesInProgressCount_;
}

void PieceStorage::SavePieceToDisk(const PiecePtr& piece){
    if(piece->HashMatches()) {
        ourFile_.seekp( piece->GetIndex() * pieceLength_);
        ourFile_ << piece->GetData();
        --piecesInProgressCount_;
        piecesSavedToDiscIndices_.push_back(piece->GetIndex());
    }else{
        piece->Reset();
        remainPieces_.push(piece);
        --piecesInProgressCount_;
        throw std::runtime_error("hashes don`t matches!");
    }
}