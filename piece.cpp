#include "byte_tools.h"
#include "piece.h"
#include <iostream>
#include <algorithm>

namespace {
constexpr size_t BLOCK_SIZE = 1 << 14;
}

 Piece::Piece(size_t index, size_t length, std::string hash) : index_(index), length_(length), hash_(std::move(hash)) { 
    int std_blocks_len = length / BLOCK_SIZE;
    int ost = length % BLOCK_SIZE;
    for (int i = 0; i < std_blocks_len; i++) {
        Block newblock;
        newblock.piece = index;
        newblock.offset = i * BLOCK_SIZE;
        newblock.length = BLOCK_SIZE;
        newblock.status = Block::Status::Missing;
       
        blocks_.push_back(newblock);
    }
    if (ost) {
        Block newblock;
        newblock.piece = index;
        newblock.offset = std_blocks_len * BLOCK_SIZE;
        newblock.length = ost;
        newblock.status = Block::Status::Missing;
       
        blocks_.push_back(newblock);
    }
 }

    /*
     * Совпадает ли хеш скачанных данных с ожидаемым
     */
bool Piece::HashMatches() const {
    return hash_ == GetDataHash();
}

/*
    * Дать указатель на отсутствующий (еще не скачанный и не запрошенный) блок
    */
Block* Piece::FirstMissingBlock() {
    for (auto &b : blocks_) {
        if (b.status == Block::Status::Missing) {
            return (&b);
        }
    }
    return nullptr;
}

/*
    * Получить порядковый номер части файла
    */
size_t Piece::GetIndex() const {
    return index_;
}

/*
    * Сохранить скачанные данные для какого-то блока
    */
void Piece::SaveBlock(size_t blockOffset, std::string data) {
    int id = blockOffset / BLOCK_SIZE;

    if (id >= blocks_.size()) {
        throw std::runtime_error("Received block with invalid offset");
    }

    if (blocks_[id].length != data.size()) {
        throw std::runtime_error("got data with length not equal waited. offset: " 
        + std::to_string(blockOffset) + " and data " + data); 
    }
    blocks_[id].data = std::move(data);
    blocks_[id].status = Block::Status::Retrieved;
}

/*
    * Скачали ли уже все блоки
    */
bool Piece::AllBlocksRetrieved() const {
    for (const auto &b : blocks_) {
        if (b.status != Block::Status::Retrieved) {
            return false;
        }
    }
    return true;
}

/*
    * Получить скачанные данные для части файла
    */
std::string Piece::GetData() const {
    std::string res = "";
    for (const auto &b : blocks_) {
        res += b.data;
    }
    return res;
}

/*
    * Посчитать хеш по скачанным данным
    */
std::string Piece::GetDataHash() const {
    return CalculateSHA1(GetData());
}

/*
    * Получить хеш для части из .torrent файла
    */
const std::string& Piece::GetHash() const {
    return hash_;
}

/*
    * Удалить все скачанные данные и отметить все блоки как Missing
    */
void Piece::Reset() {
    for (auto &b : blocks_) {
        b.data = "";
        b.status = Block::Status::Missing;
    }
}