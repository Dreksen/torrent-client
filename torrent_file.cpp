#include "torrent_file.h"
#include "bencode.h"
#include <vector>
#include <openssl/sha.h>
#include <fstream>
#include <variant>
#include <sstream>


TorrentFile LoadTorrentFile(const std::string& filename) {
    std::ifstream file;
    file.open(filename, std::ios::binary);
    if (!file) {  
        throw std::runtime_error("can't open the file " + filename);
    }
    
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string buffer;
    buffer.resize(static_cast<size_t>(size));

    if (!file.read(buffer.data(), size)) {
        throw std::runtime_error("error reading file " + filename);
    }
    
    int i = 0;
    if (buffer.size() == 0 || buffer[i] != 'd') {
        throw std::runtime_error("root is not a dict " + std::to_string(i));
    }

    Bencode::Node root;
    TorrentFile TFile{};
    
    unsigned char hash[20] = {0};   
    i++;
    while (i < buffer.size() && buffer[i] != 'e') {
        Bencode::Node key = Bencode::ProcessStr(i, buffer);
        if (key.str == "info") {
            int info_start = i; 
            Bencode::Node val = Bencode::ProcessNode(i, buffer);
            int info_end = i;   

            SHA1(reinterpret_cast<const unsigned char*>(buffer.data() + info_start), info_end - info_start, hash);
            TFile.infoHash.assign(reinterpret_cast<const char*>(hash), 20);

            root.map[key.str] = val;
        } else {
            root.map[key.str] = Bencode::ProcessNode(i, buffer);
        }
    }
    
    if (TFile.infoHash.empty()) {
        throw std::runtime_error("info doesn't found " + std::to_string(i));
    }
    auto info = root.map["info"];
    if (info.type != 3) {
        throw std::runtime_error("info isn't dict " + std::to_string(i));
    }

    if (root.map.count("announce")) TFile.announce = root.map["announce"].str;
    if (root.map.count("comment"))  TFile.comment = root.map["comment"].str;

    if (info.map.count("name")) TFile.name = info.map["name"].str;
    if (info.map.count("piece length")) TFile.pieceLength = info.map["piece length"].val;
    if (info.map.count("length")) {
        TFile.length = info.map["length"].val;
    } else if (info.map.count("files")) {
        for (auto& file_node : info.map["files"].arr) {
            TFile.length += file_node.map["length"].val;
        }
    }

    if (info.map.count("pieces")) {
        std::string pieces = info.map["pieces"].str;
        for (size_t p = 0; p < pieces.size(); p += 20) {
            TFile.pieceHashes.push_back(pieces.substr(p, 20));
        }
    }

    return TFile;
}
