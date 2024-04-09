#include "piece_storage.h"
#include <iostream>
/*
* Хранилище информации о частях скачиваемого файла.
* В этом классе отслеживается информация о том, какие части файла осталось скачать
*/
 //std::queue<PiecePtr> remainPieces_;  // очередь частей файла, которые осталось скачать
PieceStorage::PieceStorage(const TorrentFile& tf){
    size_t last_piece_size = tf.length % tf.pieceLength;
    size_t remain_size = tf.pieceHashes.size();

    if(last_piece_size > 0)
        --remain_size;

    for(size_t i = 0; i < remain_size; ++i){
        remainPieces_.push(std::make_shared<Piece>(i, tf.pieceLength, tf.pieceHashes[i]));
    }

     if(last_piece_size > 0)
         remainPieces_.push(std::make_shared<Piece>(remain_size, last_piece_size, tf.pieceHashes[remain_size]));

}

    /*
     * Отдает указатель на следующую часть файла, которую надо скачать
     */
PiecePtr PieceStorage::GetNextPieceToDownload() {
    std::cout << "IN PieceStorage::GetNextPieceToDownload" << std::endl;
    PiecePtr temp = remainPieces_.front();
    remainPieces_.pop();
    return temp;/////???????????????????????
}

/*
     * Эта функция вызывается из PeerConnect, когда скачивание одной части файла завершено.
     * В рамках данного задания требуется очистить очередь частей для скачивания как только хотя бы одна часть будет успешно скачана.
     */
void PieceStorage::PieceProcessed(const PiecePtr& piece) {
    while(!this->QueueIsEmpty()){
        remainPieces_.pop();
    }
}

/*
     * Остались ли нескачанные части файла?
     */
bool PieceStorage::QueueIsEmpty() const {
    return remainPieces_.empty();
}

/*
     * Сколько частей файла всего
     */
size_t PieceStorage::TotalPiecesCount() const {
    return remainPieces_.size();
}

void PieceStorage::SavePieceToDisk(PiecePtr piece) {
    // Эта функция будет переопределена при запуске вашего решения в проверяющей системе
    // Вместо сохранения на диск там пока что будет проверка, что часть файла скачалась правильно
    std::cout << "Downloaded piece " << piece->GetIndex() << std::endl;
    std::cerr << "Clear pieces list, don't want to download all of them" << std::endl;
    while (!remainPieces_.empty()) {
        remainPieces_.pop();
    }
}
