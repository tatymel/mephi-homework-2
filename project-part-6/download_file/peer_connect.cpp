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
    return (((bitfield_[bytes] >> offset) & 1) == 1);
}

void PeerPiecesAvailability::SetPieceAvailability(size_t pieceIndex){
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
        failed_(false),
        pieceStorage_(pieceStorage),
        pieceInProgress_(nullptr){
}

void PeerConnect::Run() {
    while (!terminated_) {
        if (EstablishConnection()) {
            std::cout << "Connection established to peer" << std::endl;
            MainLoop();
        } else {
            std::cerr << "Cannot establish connection to peer" << std::endl;
            Terminate();
        }
        socket_.CloseConnection();
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
        answer = socket_.ReceiveData();

    }
    Message ms = Message::Parse(answer);
    if (ms.id == MessageId::Unchoke) {
        choked_ = false;
    }else if (ms.id == MessageId::BitField) {
        choked_ = false;
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


void PeerConnect::RequestPiece() {
    if(piecesAvailability_.Size() == 0){
        terminated_ = true;
        return;
    }

    if(pieceInProgress_ == nullptr){ ////choose piece
        pieceInProgress_ = pieceStorage_.GetNextPieceToDownload();
        if(pieceInProgress_ == nullptr){ //we have downloaded all
            terminated_ = true;
            return;
        }
        while(!piecesAvailability_.IsPieceAvailable(pieceInProgress_->GetIndex())){////never happen but have to be thought
            pieceStorage_.AddPieceToQueue(pieceInProgress_);
            pieceInProgress_->Reset();
            pieceInProgress_ = pieceStorage_.GetNextPieceToDownload();
        }
    }


    Message ms = Message::Init(MessageId::Request,
                               IntToBytes(pieceInProgress_->GetIndex())
                               + IntToBytes(pieceInProgress_->FirstMissingBlock()->offset)
                               + IntToBytes(pieceInProgress_->FirstMissingBlock()->length));
    pieceInProgress_->FirstMissingBlock()->status = Block::Status::Pending;
    pendingBlock_ = true;
    try {
        socket_.SendData(ms.ToString());
    }catch(std::exception& ex){
        pieceStorage_.AddPieceToQueue(pieceInProgress_);
        pieceInProgress_->Reset();
        failed_ = true;
        terminated_ = true;
    }
}

void PeerConnect::Terminate() {
    std::cerr << "Terminate" << std::endl;
    terminated_ = true;
}

bool PeerConnect::Failed() const{
    return failed_;
}

void PeerConnect::MainLoop() {
    while (!terminated_) {
        std::string answer;
        try {
            answer = socket_.ReceiveData();
        }catch (std::exception& ex){
            if(pieceInProgress_ != nullptr) {
                pieceStorage_.AddPieceToQueue(pieceInProgress_);
                pieceInProgress_->Reset();
            }
            failed_ = true;
            terminated_ = true;
            break;
        }
        Message ms = Message::Parse(answer);

        if(ms.id == MessageId::Unchoke){
            choked_ = false;

        }else if(ms.id == MessageId::Choke){
            if(pieceInProgress_ != nullptr) {
                pieceStorage_.AddPieceToQueue(pieceInProgress_);
                pieceInProgress_->Reset();
            }
            choked_ = true;
            terminated_ = true;
            failed_ = true;

        }else if(ms.id == MessageId::Have){
            int piece_index = BytesToInt(ms.payload.substr(0, 4));
            piecesAvailability_.SetPieceAvailability(piece_index);

        }else if(ms.id == MessageId::Piece){
            int piece_index = BytesToInt(ms.payload.substr(0, 4));
            int offset = BytesToInt(ms.payload.substr(4, 4));

            if(piece_index != pieceInProgress_->GetIndex()){
                if(pieceInProgress_ != nullptr) {
                    pieceStorage_.AddPieceToQueue(pieceInProgress_);
                    pieceInProgress_->Reset();
                }
                failed_ = true;
                terminated_ = true;
            }

            pieceInProgress_->SaveBlock(offset, ms.payload.substr(8, ms.messageLength - 9));
            if(pieceInProgress_->AllBlocksRetrieved()){
                pieceStorage_.PieceProcessed(pieceInProgress_);
                terminated_ = true;
                break;
            }
            pendingBlock_ = false;
        }
        if (!choked_ && !pendingBlock_) {
            RequestPiece();
        }
    }
}
