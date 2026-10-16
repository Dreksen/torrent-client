#include "piece_storage.h"
#include <iostream>


PieceStorage::PieceStorage(const TorrentFile& tf) {
    for (size_t i = 0; i < tf.pieceHashes.size(); i++) {
        int length = std::min(tf.pieceLength, tf.length - i * tf.pieceLength);
        Piece p(i, length, tf.pieceHashes[i]); 
        remainPieces_.push(std::make_shared<Piece>(p));
    }
    totalPieces_ = tf.pieceHashes.size();
}

PiecePtr PieceStorage::GetNextPieceToDownload() {
    if (QueueIsEmpty()) {
        return nullptr;
    }
    return remainPieces_.front();
}

void PieceStorage::PieceProcessed(const PiecePtr& piece) {
    SavePieceToDisk(piece);
}

bool PieceStorage::QueueIsEmpty() const {
    return remainPieces_.empty();
}

size_t PieceStorage::TotalPiecesCount() const {
    return totalPieces_;
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
