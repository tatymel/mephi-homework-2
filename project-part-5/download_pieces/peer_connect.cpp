#include "byte_tools.h"
#include "peer_connect.h"
#include "message.h"
#include <iostream>
#include <sstream>
#include <utility>
#include <cassert>

using namespace std::chrono_literals;
PeerPiecesAvailability::PeerPiecesAvailability() = default;
PeerPiecesAvailability::PeerPiecesAvailability(std::string bitfield) : bitfield_(std::move(bitfield)){}

bool PeerPiecesAvailability::IsPieceAvailable(size_t pieceIndex) const{
    size_t bytes = pieceIndex / 8;
    size_t offset = 7 - pieceIndex % 8;
    std::cout << "in IsPieceAvailable and return: " << (((bitfield_[bytes] >> offset) & 1) == 1)<< std::endl;
    return (((bitfield_[bytes] >> offset) & 1) == 1);
}

void PeerPiecesAvailability::SetPieceAvailability(size_t pieceIndex){
    std::cout << "in SetPieceAvailability" << std::endl;
    size_t bytes = pieceIndex / 8;
    size_t offset = 7 - pieceIndex % 8;
    bitfield_[bytes] = char(bitfield_[bytes] | (1 << offset));
}

size_t PeerPiecesAvailability::Size() const{
    size_t res = 0;
    for (size_t i = 0; i < bitfield_.size(); ++i) {
        for (size_t bit = 0; bit < 8; ++bit) {
            if (((bitfield_[i] >> bit) & 1) == 1) {
                ++res;
            }
        }
    }
    return res;
}

PeerConnect::PeerConnect(const Peer& peer, const TorrentFile &tf, std::string selfPeerId, PieceStorage& pieceStorage) :
        tf_(tf),
        selfPeerId_(std::move(selfPeerId)),
        socket_(TcpConnect(peer.ip, peer.port, 2000ms, 2000ms)),
        terminated_(false),
        choked_(true),
        pendingBlock_(false),
        pieceStorage_(pieceStorage),
        pieceInProgress_(nullptr){
}
/*
   * Основная функция, в которой будет происходить цикл общения с пиром.
   * https://wiki.theory.org/BitTorrentSpecification#Messages
   */
void PeerConnect::Run() {
    while (!terminated_) {
        if (EstablishConnection()) {
            std::cout << "Connection established to peer" << std::endl;
            MainLoop();
        } else {
            std::cerr << "Cannot establish connection to peer" << std::endl;
            Terminate();
        }
    }
}

void PeerConnect::PerformHandshake() {
    //* - Подключиться к пиру по протоколу TCP
    socket_.EstablishConnection();

    //* - Отправить пиру сообщение handshake
    std::string firstMessage =  "BitTorrent protocol00000000" + tf_.infoHash + selfPeerId_;
    firstMessage = ((char)(19)) + firstMessage;
    socket_.SendData(firstMessage);

    //* - Проверить правильность ответа пира
    std::string answer = socket_.ReceiveData(68);
    std::string infoHashPeers = std::string(&answer[28], 20);
    if(infoHashPeers != tf_.infoHash){
        throw std::exception();
    }
    peerId_ = std::string(&answer[48], 20);
}

bool PeerConnect::EstablishConnection() {
    try {
        PerformHandshake();
        ReceiveBitfield();
        SendInterested();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to establish connection with peer " << socket_.GetIp() << ":" <<
            socket_.GetPort() << " -- " << e.what() << std::endl;
        return false;
    }
}

void PeerConnect::ReceiveBitfield() {
    std::string answer = socket_.ReceiveData();
    if ( (int(uint8_t(answer[0])) > 9)) {//meta data
        std::cout << "METADATA" << std::endl;
        answer = socket_.ReceiveData();

    }
    Message ms = Message::Parse(answer);
    if (ms.id == MessageId::Unchoke) {
        choked_ = false;
    }else if (ms.id == MessageId::BitField) {
        choked_ = false;
        std::cout << "MessageId::BitField has been caught!!!!!!!!" << std::endl;
        piecesAvailability_ = std::move(PeerPiecesAvailability(ms.payload));
    } else if(ms.id == MessageId::Choke){
        choked_ = true;
    }
}

void PeerConnect::SendInterested() {
    Message ms({MessageId::Interested, 1, ""});
    std::string interestInFormat = ms.ToString();
    socket_.SendData(interestInFormat);
}

/*
     * Функция отправляет пиру сообщение типа request. Это сообщение обозначает запрос части файла у пира.
     * За одно сообщение запрашивается не часть целиком, а блок данных размером 2^14 байт или меньше.
     * Если в данный момент мы не знаем, какую часть файла надо запросить у пира, то надо получить эту информацию у
     * PieceStorage
     */
void PeerConnect::RequestPiece() {

    if(piecesAvailability_.Size() == 0){
        terminated_ = true;
        std::cout << "piecesAvailability_.Size() == 0" << std::endl;
        return; ///??????????
    }
    std::cout << "IN PeerConnect::RequestPiece" << std::endl;

    if(pieceInProgress_ == nullptr){
        pieceInProgress_ = pieceStorage_.GetNextPieceToDownload();
        while(!piecesAvailability_.IsPieceAvailable(pieceInProgress_->GetIndex())){
            std::cout << "peer doesn`t have " << pieceInProgress_->GetIndex() << " piece" << std::endl;
            pieceInProgress_ = pieceStorage_.GetNextPieceToDownload();
        }
    }
    std::cout << "pieceInProgress_->GetIndex(): " << pieceInProgress_->GetIndex() << std::endl;

    Message ms = Message::Init(MessageId::Request,
                               IntToBytes(pieceInProgress_->GetIndex())
                               + IntToBytes(pieceInProgress_->FirstMissingBlock()->offset)
                               + IntToBytes(pieceInProgress_->FirstMissingBlock()->length));
    pieceInProgress_->FirstMissingBlock()->status = Block::Status::Pending;
    pendingBlock_ = true;
    socket_.SendData(ms.ToString());
}

void PeerConnect::Terminate() {
    std::cerr << "Terminate" << std::endl;
    terminated_ = true;
}

/*
    * Основной цикл общения с пиром. Здесь мы ждем следующее сообщение от пира и обрабатываем его.
    * Также, если мы не ждем в данный момент от пира содержимого части файла, то надо отправить соответствующий запрос
    */
void PeerConnect::MainLoop() {
    while (!terminated_) {
        // сюда писать код
        std::string answer = socket_.ReceiveData();
        Message ms = Message::Parse(answer);
        if(ms.id == MessageId::Unchoke){
            std::cout << "get Message Unchoke" << std::endl;

            choked_ = false;

        }else if(ms.id == MessageId::Choke){/////?????????????
            std::cout << "get Message Choke" << std::endl;

            choked_ = true;
            terminated_ = true;

        }else if(ms.id == MessageId::Have){
            std::cout << "get Message Have" << std::endl;

            int piece_index = BytesToInt(ms.payload.substr(0, 4));
            piecesAvailability_.SetPieceAvailability(piece_index);

        }else if(ms.id == MessageId::Piece){
            std::cout << "get Message Piece" << std::endl;

            int piece_index = BytesToInt(ms.payload.substr(0, 4));
            int offset = BytesToInt(ms.payload.substr(4, 4));

            if(piece_index != pieceInProgress_->GetIndex()){
                std::cout << piece_index << " pieceInProgress_->GetIndex(): " << pieceInProgress_->GetIndex() << std::endl;
                throw std::runtime_error("piece_index != pieceInProgress_->GetIndex()");
            }

            pieceInProgress_->SaveBlock(offset, ms.payload.substr(8, ms.messageLength - 9));
            std::cout << "Right now we pieceInProgress_->SaveBlock, piece_index: " << pieceInProgress_->GetIndex()
            << ", offset: " << offset << ", piece_index: " << piece_index << std::endl;

            if(pieceInProgress_->AllBlocksRetrieved()){
                std::cout << "pieceInProgress_->AllBlocksRetrieved() = true" << std::endl;
                pieceStorage_.PieceProcessed(pieceInProgress_);
                terminated_ = true;
                break;//////////////
            }

            pendingBlock_ = false;
        }
        if (!choked_ && !pendingBlock_) {
            RequestPiece();
        }
    }
}
