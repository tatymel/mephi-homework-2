#include "tcp_connect.h"
#include "byte_tools.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <chrono>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits>
#include <utility>
using namespace std::chrono_literals;
const std::string &TcpConnect::GetIp() const {
    return ip_;
}

int TcpConnect::GetPort() const {
    return port_;
}

TcpConnect::TcpConnect(std::string ip, int port, std::chrono::milliseconds connectTimeout, std::chrono::milliseconds readTimeout)
        : ip_(std::move(ip)),
          port_(port),
          connectTimeout_(connectTimeout),
          readTimeout_(readTimeout){}

TcpConnect::~TcpConnect()= default;

void TcpConnect::EstablishConnection(){
    sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sock_ < 0){
        throw std::runtime_error("Failed during setting socket");
    }
    int fl = fcntl(sock_, F_GETFL, 0);

    if(fcntl(sock_,F_SETFL, fl | O_NONBLOCK) < 0){
        throw std::runtime_error("Failed during fcntl");
    }//Установив сокет на не-блокировку, вы можете эффективно его опрашивать.


    struct sockaddr_in peer_addr{};
    memset(&peer_addr, 0, sizeof(peer_addr));
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(port_);
    inet_aton(ip_.c_str(), &peer_addr.sin_addr);
    /* inet_aton() возвращает не-ноль, если адрес действительный, и ноль, если нет.

     inet_ntoa() возвращает строку цифр-и-точек в статическом буфере, который при
     каждом вызове функции перезаписывается.

     inet_addr() возвращает адрес в формате in_addr_t, или -1 при ошибке. (Тот же
     результат вы получите, если попытаетесь преобразовать строку “255.255.255.255”, которая является действительным IP адресом. Вот почему inet_aton() лучше.)*/

    if(connect(sock_, (struct sockaddr*)&peer_addr, sizeof(peer_addr)) < 0){
        if (errno != EINPROGRESS)
        {
            throw std::runtime_error("Failed during connecting through socket");
        }
    }

    fd_set IsReady{};
    struct timeval Timeval{};

    FD_ZERO(&IsReady);//заранее очищаем массив
    // добавляем наш дескриптор в массив
    FD_SET(sock_, &IsReady);

    // ждём появления данных на каком-либо сокете (таймаут connectTimeout секунд)
    Timeval.tv_sec = connectTimeout_.count() / 1'000;
    Timeval.tv_usec = (connectTimeout_.count() % 1'000) * 1'000;
    auto rv = select(sock_ + 1, NULL, &IsReady, NULL, &Timeval);

    if(rv == -1){
        throw std::runtime_error("Failed during select");
    } else if(rv == 0){
        throw std::runtime_error("Failed: connectTimeout");
    }

    int t = 0;
    socklen_t sizeT = sizeof(t);
    if(getsockopt(sock_, SOL_SOCKET, SO_ERROR, &t, &sizeT) < 0){
        throw std::runtime_error("Failed: getsockopt < 0");
    }else if(t != 0){
        throw std::runtime_error("Failed: getsockopt != 0");
    }
    fcntl(sock_, F_SETFL,fl & ~O_NONBLOCK);
}

void TcpConnect::SendData(const std::string& data) const{
    const char* data_ = data.c_str();
    size_t size_data = data.size();
    while(size_data > 0){
        long res = send(sock_, data_, size_data, 0);
        if(res < 0){
            throw std::runtime_error("Failed during sendData");
        }
        data_ += res;
        size_data -= res;
    }
}

std::string TcpConnect::ReceiveData(size_t bufferSize) const {
    struct pollfd Poll[1];
    // хотим узнать когда обычные или out-of-band // готовы к приёму (recv())...

/*    POLLIN - Известить меня о готовности данных к recv() на этом сокете.
    POLLOUT - Известить меня, что я могу вызвать send() на этом сокете без блокировки.
    POLLPRI -  Известить меня о готовности out-of-band данных к recv().*/

    Poll[0].fd = sock_;
    Poll[0].events = POLLIN | POLLPRI;

    int feedbackPoll = poll(&Poll[0], 1, (int) readTimeout_.count());
    if (feedbackPoll < 0) {
        throw std::runtime_error("Failed during poll");
    } else if (feedbackPoll == 0) {
        throw std::runtime_error("Failed: readTimeout");
    }
    size_t bufferRealSize = bufferSize;
    if (bufferSize == 0) {
        char buf_size[4];
        long res = recv(sock_, &buf_size, 4, 0);
        if (res != 4) {
            throw std::runtime_error("Failed during getting realSize of data");
        }
        bufferRealSize = BytesToInt(std::string_view(buf_size));
    }


    std::string buffer = std::string( bufferRealSize, '0');

    long already_has = 0, res;
    while (already_has < bufferRealSize) {
        res = recv(sock_, &buffer[already_has], bufferRealSize - already_has, 0);
        if (res < 0 || res == 0) {
            throw std::runtime_error("Failed during getting data");
        }
        already_has += res;
    }
    return buffer;
}

void TcpConnect::CloseConnection(){
    close(TcpConnect::sock_);
}

