#include "torrent_tracker.h"
#include "bencode.h"
#include "byte_tools.h"
#include <iostream>
#include <cpr/cpr.h>

const std::vector<Peer> &TorrentTracker::GetPeers() const {
    return peers_;
}


TorrentTracker::TorrentTracker(const std::string& url) : url_(url) {}

void TorrentTracker::UpdatePeers(const TorrentFile& tf, std::string peerId, int port) {
    namespace fs = std::filesystem;

    cpr::Response res = cpr::Get(
            cpr::Url{url_},
            cpr::Parameters {
                    {"info_hash", tf.infoHash},
                    {"peer_id", peerId},
                    {"port", std::to_string(port)},
                    {"uploaded", std::to_string(0)},
                    {"downloaded", std::to_string(0)},
                    {"left", std::to_string(tf.length)},
                    {"compact", std::to_string(1)}
            },
            cpr::Timeout{20000}
    );

    if (res.status_code == 200) {
        if (res.text.find("failure reason") == std::string::npos) {
            std::cout << "Successfully requested peers from " << url_ << std::endl;
            int i = 0;
            auto resNode = Bencode::ProcessNode(i, res.text);
            if (resNode.type != 3) {
                throw std::runtime_error("res is not a dict");
            }
            if (resNode.map["peers"].type != 1) {
                throw std::runtime_error("res[peers] is not a string, but " + std::to_string(resNode.map["peers"].type));
            }

            std::string peersStr = resNode.map["peers"].str;
            for (int j = 0; j + 5 < peersStr.size(); j += 6) {
                std::string_view ip_view(peersStr.data() + j, 4);
                int ip_num = BytesToInt(ip_view);

                std::string ip = std::to_string((ip_num >> 24) & 0xFF) + "." + 
                     std::to_string((ip_num >> 16) & 0xFF) + "." + 
                     std::to_string((ip_num >> 8) & 0xFF) + "." + 
                     std::to_string(ip_num & 0xFF);

                unsigned char p1 = static_cast<unsigned char>(peersStr[j + 4]);
                unsigned char p2 = static_cast<unsigned char>(peersStr[j + 5]);
                int port = (p1 << 8) | p2;

                peers_.push_back(Peer{ip, port});
            }
            return;
        } else {
            std::cout << "Server responded '" << res.text << "'" << std::endl;
        }
    }

    std::cout << "Possibly you have parsed a .torrent file wrong hence the request for peers is wrong too" << std::endl;
    std::cout << "Tried GET " << res.url << std::endl;
    std::cout << "Error: " << (int)res.error.code << " - " << res.error.message << std::endl;
    if (res.error.code != cpr::ErrorCode::EMPTY_RESPONSE) {
        std::cout << "Tracker response: " << res.status_code << " " << res.status_line << ": " << res.text << std::endl;
    }

    std::cout << "Info hash: ";
    for (char c : tf.infoHash) {
        std::cout << (unsigned int)(unsigned char)c << " ";
    }
    std::cout << std::endl;

    throw std::runtime_error("status code != 200 ");    
}
