#include "torrent_tracker.h"
#include "bencode.h"
#include "byte_tools.h"
#include <cpr/cpr.h>

TorrentTracker::TorrentTracker(const std::string& url) : url_(std::move(url)){}

const std::vector<Peer> &TorrentTracker::GetPeers() const {
    return peers_;
}
void TorrentTracker::UpdatePeers(const TorrentFile& tf, std::string peerId, int port){
    cpr::Response res = cpr::Get(
            cpr::Url{tf.announce},
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

    std::stringstream content;
    content << res.text;
    Bencode::DictType dictionary;


    std::string info;
    Bencode::MapString mp;
    Bencode::ForInfo fi;
    RecursiveParsing(content, dictionary, "FIRST_MAP", mp,  fi);

    std::stringstream peers_str;
    peers_str << std::get<std::string>(dictionary["peers"]);
    for(size_t i = 0; i < std::get<std::string>(dictionary["peers"]).size(); i+=6){
        std::string ip_;
        for (size_t j = 0; j < 4; ++j) {
            ip_ += std::to_string(((int) peers_str.get()));
            if (j < 3)
                ip_ += ".";
        }
        auto port_ = static_cast<uint16_t>((int)  peers_str.get() << 8) | static_cast<uint16_t>((int)  peers_str.get());
        peers_.push_back({ip_, port_});
    }
}
