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
    std::cout << "in Size and size:" << res << std::endl;
    return res;
}

PeerConnect::PeerConnect(const Peer& peer, const TorrentFile &tf, std::string selfPeerId)
                : tf_(tf),
                selfPeerId_(std::move(selfPeerId)),
                socket_(TcpConnect(peer.ip, peer.port, 1000ms, 1000ms))
    {
        terminated_ = false;
        choked_ = true;
    }

void PeerConnect::Run() {
    while (!terminated_) {
        if (EstablishConnection()) {
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
        std::cout << "try to PerformHandshake()" << std::endl;
        PerformHandshake();
        std::cout << "try to ReceiveBitfield()" << std::endl;
        ReceiveBitfield();
        std::cout << "try to SendInterested()" << std::endl;
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
    }
}

void PeerConnect::SendInterested() {
    Message ms({MessageId::Interested, 1, ""});
    std::string interestInFormat = ms.ToString();
    socket_.SendData(interestInFormat);
}

void PeerConnect::Terminate() {
    std::cerr << "Terminate" << std::endl;
    terminated_ = true;
}

void PeerConnect::MainLoop() {
    /*
     * При проверке вашего решения на сервере этот метод будет переопределен.
     * Если вы провели handshake верно, то в этой функции будет работать обмен данными с пиром
     */
    std::cout << "Dummy main loop" << std::endl;
    Terminate();
}
