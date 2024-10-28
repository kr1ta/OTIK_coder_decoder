#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <unordered_map>

struct Node
{
    uint64_t frequency;
    uint8_t byte;
    Node* left;
    Node* right;

    Node(uint64_t freq, uint8_t b) : frequency(freq), byte(b), left(nullptr), right(nullptr) {}
};

struct Compare
{
    bool operator()(Node* l, Node* r)
    {
        return l->frequency > r->frequency;
    }
};

void generateCodes(Node* root, std::string str, std::unordered_map<uint8_t, std::string>& huffmanCodes);

Node* buildHuffmanTree(const std::vector<uint64_t>& byteFrequency);

void countByteFrequency(const std::string& inputFileName, std::vector<uint64_t>& byteFrequency);

void writeFrequencyTable(const std::string& outputFileName, const std::vector<uint64_t>& byteFrequency);

void readFrequencyTable(const std::string& inputFileName, std::vector<uint64_t>& byteFrequency, int offset);

uint32_t compressFile(const std::string& inputFileName, const std::string& outputFileName, const std::vector<uint64_t>& byteFrequency, std::unordered_map<uint8_t, std::string> huffmanCodes);

int decompressFile(const std::string& inputFileName, const std::string& outputFileName, const std::vector<uint64_t>& byteFrequency, std::unordered_map<uint8_t, std::string> huffmanCodes, int offset);

void printHuffmanCode(std::unordered_map<uint8_t, std::string> huffmanCodes);

#endif