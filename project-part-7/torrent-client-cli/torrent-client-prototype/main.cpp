#include "torrent_tracker.h"
#include "piece_storage.h"
#include "peer_connect.h"
#include "byte_tools.h"
#include <cassert>
#include <iostream>
#include <filesystem>
#include <random>
#include <thread>
#include <algorithm>

namespace fs = std::filesystem;

std::string RandomString(size_t length) {
    std::random_device random;
    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        result.push_back(random() % ('Z' - 'A') + 'A');
    }
    return result;
}

const std::string PeerId = "TESTAPPDONTWORRY" + RandomString(4);
size_t PiecesToDownload;

void CheckDownloadedPiecesIntegrity(const std::filesystem::path& outputFilename, const TorrentFile& tf, PieceStorage& pieces) {
    pieces.CloseOutputFile();

    if (std::filesystem::file_size(outputFilename) != tf.length) {
        throw std::runtime_error("Output file has wrong size");
    }

    if (pieces.GetPiecesSavedToDiscIndices().size() != pieces.PiecesSavedToDiscCount()) {
        throw std::runtime_error("Cannot determine real amount of saved pieces");
    }

    if (pieces.PiecesSavedToDiscCount() < PiecesToDownload) {
        throw std::runtime_error("Downloaded pieces amount is not enough");
    }

    if (pieces.TotalPiecesCount() != tf.pieceHashes.size() || pieces.TotalPiecesCount() < 200) {
        throw std::runtime_error("Wrong amount of pieces");
    }

    std::vector<size_t> pieceIndices = pieces.GetPiecesSavedToDiscIndices();
    std::sort(pieceIndices.begin(), pieceIndices.end());

    std::ifstream file(outputFilename, std::ios_base::binary);
    for (size_t pieceIndex : pieceIndices) {
        const std::streamoff positionInFile = pieceIndex * tf.pieceLength;
        file.seekg(positionInFile);
        if (!file.good()) {
            throw std::runtime_error("Cannot read from file");
        }
        std::string pieceDataFromFile(tf.pieceLength, '\0');
        file.read(pieceDataFromFile.data(), tf.pieceLength);
        const size_t readBytesCount = file.gcount();
        pieceDataFromFile.resize(readBytesCount);
        const std::string realHash = CalculateSHA1(pieceDataFromFile);

        if (realHash != tf.pieceHashes[pieceIndex]) {
            std::cerr << "File piece with index " << pieceIndex << " started at position " << positionInFile <<
                      " with length " << pieceDataFromFile.length() << " has wrong hash " << HexEncode(realHash) <<
                      ". Expected hash is " << HexEncode(tf.pieceHashes[pieceIndex]) << std::endl;
            throw std::runtime_error("Wrong piece hash");
        }
    }
}



bool RunDownloadMultithread(PieceStorage& pieces, const TorrentFile& torrentFile, const std::string& ourId, const TorrentTracker& tracker) {
    using namespace std::chrono_literals;

    std::vector<std::thread> peerThreads;
    std::vector<PeerConnect> peerConnections;

    for (const Peer& peer : tracker.GetPeers()) {
        peerConnections.emplace_back(peer, torrentFile, ourId, pieces);
    }

    for (PeerConnect& peerConnect : peerConnections) {
        peerThreads.emplace_back(
                [&peerConnect] () {
                    bool tryAgain = true;
                    int attempts = 0;
                    do {
                        try {
                            ++attempts;
                            peerConnect.Run();
                        } catch (const std::runtime_error& e) {
                            std::cerr << "Runtime error: " << e.what() << std::endl;
                        } catch (const std::exception& e) {
                            std::cerr << "Exception: " << e.what() << std::endl;
                        } catch (...) {
                            std::cerr << "Unknown error" << std::endl;
                        }
                        tryAgain = peerConnect.Failed() && attempts < 3;
                    } while (tryAgain);
                }
        );
    }

    std::this_thread::sleep_for(10s);
    while (pieces.PiecesSavedToDiscCount() < PiecesToDownload) {
        if (pieces.PiecesInProgressCount() == 0) {
            std::cerr << "Want to download more pieces but all peer connections are not working. Let's request new peers" << std::endl;
            for (PeerConnect& peerConnect : peerConnections) {
                peerConnect.Terminate();
            }
            for (std::thread& thread : peerThreads) {
                thread.join();
            }
            return true;
        }
        std::this_thread::sleep_for(1s);
    }

    for (PeerConnect& peerConnect : peerConnections) {
        peerConnect.Terminate();
    }

    for (std::thread& thread : peerThreads) {
        thread.join();
    }

    return false;
}

void DownloadTorrentFile(const TorrentFile& torrentFile, PieceStorage& pieces, const std::string& ourId) {
    std::cout << "Connecting to tracker " << torrentFile.announce << std::endl;
    TorrentTracker tracker(torrentFile.announce);
    bool requestMorePeers = false;
    do {
        tracker.UpdatePeers(torrentFile, ourId, 12345);

        if (tracker.GetPeers().empty()) {
            std::cerr << "No peers found. Cannot download a file" << std::endl;
        }

        std::cout << "Found " << tracker.GetPeers().size() << " peers" << std::endl;
        for (const Peer& peer : tracker.GetPeers()) {
            std::cout << "Found peer " << peer.ip << ":" << peer.port << std::endl;
        }

        requestMorePeers = RunDownloadMultithread(pieces, torrentFile, ourId, tracker);
    } while (requestMorePeers);
}



int main(int argc, char* argv[]) {
    std::filesystem::path outputDirectory;
    double percentToDownload;
    std::filesystem::path pathToTorrentFile;

    char option;
    while ((option = getopt(argc, argv, "d:p:")) != -1) {
        if(option == 'd') {
            outputDirectory = optarg;
        }else if(option == 'p') {
            percentToDownload = std::stod(optarg) / 100;
        }
    }

    if (optind < argc) { //(int)optind - index next элемента массива argv
        pathToTorrentFile = argv[optind];
    }else{
        std::cerr << "Wrong query line" << std::endl;
        return 1;
    }


    TorrentFile torrentFile;
    try {
        torrentFile = LoadTorrentFile(pathToTorrentFile);
        PiecesToDownload = torrentFile.length / torrentFile.pieceLength * percentToDownload;
        std::cout << "PiecesToDownload: " << PiecesToDownload << std::endl;
        std::cout << "Loaded torrent file " << pathToTorrentFile << ". " << torrentFile.comment << std::endl;
    } catch (const std::invalid_argument& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    PieceStorage pieces(torrentFile, outputDirectory, PiecesToDownload);

    DownloadTorrentFile(torrentFile, pieces, PeerId);

    CheckDownloadedPiecesIntegrity(outputDirectory / torrentFile.name, torrentFile, pieces);

    return 0;
}
