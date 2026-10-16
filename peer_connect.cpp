#include "byte_tools.h"
#include "peer_connect.h"
#include "message.h"
#include "tcp_connect.h"
#include "piece_storage.h"
#include <iostream>
#include <sstream>
#include <utility>
#include <cassert>

using namespace std::chrono_literals;

PeerConnect::PeerConnect(const Peer& peer, const TorrentFile &tf, std::string selfPeerId, PieceStorage PieceStorage) : 
    peerId_(peer.ip), tf_(tf), selfPeerId_(selfPeerId), socket_(peer.ip, peer.port, 1000ms, 1000ms), pieces_(PieceStorage) { }

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

void PeerConnect::PerformHandshake() {
    socket_.EstablishConnection();

    std::string data = std::string(1, char(19)) + "BitTorrent protocol" + std::string(8, '\0') + tf_.infoHash + selfPeerId_;
    socket_.SendData(data);

    std::string got_data = socket_.ReceiveData(68);

    std::string peer_info_hash = got_data.substr(28, 20);
    if (peer_info_hash != tf_.infoHash) {
        socket_.CloseConnection(); 
        throw std::runtime_error("wrong infohash of peer ");
    }
    peerId_ = got_data.substr(48, 20);
}

void PeerConnect::ReceiveBitfield() {
    std::string rawData = socket_.ReceiveData(0);
    Message msg = Message::Parse(rawData);
    while (msg.id == MessageId::KeepAlive) {
        rawData = socket_.ReceiveData(0);
        msg = Message::Parse(rawData);
    }

    if (msg.id == MessageId::BitField) {
        piecesAvailability_ = PeerPiecesAvailability(msg.payload);
    } 
    else if (msg.id == MessageId::Unchoke) {
        choked_ = false;
    } 
    else {
        throw std::runtime_error("wrong msgID : " + std::to_string(static_cast<int>(msg.id)));
    }
}

void PeerConnect::SendInterested() {
    Message msg = Message::Init(MessageId::Interested, "");
    socket_.SendData(msg.ToString());
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




PeerPiecesAvailability::PeerPiecesAvailability() {
    bitfield_ = "";
}

/*
    * bitfield -- массив байтов, в котором i-й бит означает наличие или отсутствие i-й части файла у пира
    * https://wiki.theory.org/BitTorrentSpecification#bitfield:_.3Clen.3D0001.2BX.3E.3Cid.3D5.3E.3Cbitfield.3E
    */
PeerPiecesAvailability::PeerPiecesAvailability(std::string bitfield) : bitfield_(std::move(bitfield)) {}


bool PeerPiecesAvailability::IsPieceAvailable(size_t pieceIndex) const {
    int byteIndex = pieceIndex / 8;
    if (byteIndex >= bitfield_.size()) {
        return false; 
    }
    int mask = (1 << (7 - pieceIndex % 8));
    return bitfield_[byteIndex] & mask;
}

void PeerPiecesAvailability::SetPieceAvailability(size_t pieceIndex) {
    int byteIndex = pieceIndex / 8;
    if (byteIndex >= bitfield_.size()) {
        bitfield_.resize(byteIndex + 1, '\0');
    }
    int mask = (1 << (7 - pieceIndex % 8));
    bitfield_[byteIndex] |= mask;
}

size_t PeerPiecesAvailability::Size() const {
    return bitfield_.size() * 8;
}