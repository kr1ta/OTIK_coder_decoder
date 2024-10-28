#include <iostream>
#include <fstream>
#include <queue>
#include <bitset>
#include <stdio.h> 
#include "huffman.h"
#define WHERESTART 2048

using namespace std;

void generateCodes(Node* root, string str, unordered_map<uint8_t, string>& huffmanCodes)
{
	if (!root) return;
	if (root->left == nullptr && root->right == nullptr)
	{
		huffmanCodes[root->byte] = str;
	}

	generateCodes(root->left, str + "0", huffmanCodes);
	generateCodes(root->right, str + "1", huffmanCodes);
}

Node* buildHuffmanTree(const vector<uint64_t>& byteFrequency)
{
	priority_queue<Node*, vector<Node*>, Compare> minHeap;

	for (int i = 0; i < 256; i++)
	{
		if (byteFrequency[i] > 0)
		{
			minHeap.push(new Node(byteFrequency[i], i));
		}
	}

	while (minHeap.size() > 1)
	{
		Node* left = minHeap.top(); minHeap.pop();
		Node* right = minHeap.top(); minHeap.pop();
		Node* newNode = new Node(left->frequency + right->frequency, 0);
		newNode->left = left;
		newNode->right = right;
		minHeap.push(newNode);
	}

	return minHeap.top();
}

void countByteFrequency(const string& inputFileName, vector<uint64_t>& byteFrequency)
{
	ifstream inputFile(inputFileName, ios::binary);
	if (!inputFile)
	{
		cerr << "Failed to open the file for reading!" << endl;
		exit(1);
	}

	char byte;
	while (inputFile.get(byte))
	{
		byteFrequency[static_cast<unsigned char>(byte)]++;
	}

	inputFile.close();
}

void writeFrequencyTable(const string& outputFileName, const vector<uint64_t>& byteFrequency)
{
	ofstream outputFile;
	outputFile.open(outputFileName, ios::binary | ios::app);

	if (!outputFile)
	{
		cerr << "Failed to open the file for writing!" << endl;
		exit(1);
	}

	for (const auto& freq : byteFrequency)
	{
		outputFile.write(reinterpret_cast<const char*>(&freq), sizeof(freq));
	}

	outputFile.close();
}

void readFrequencyTable(const std::string& inputFileName, std::vector<uint64_t>& byteFrequency, int offset)
{
	std::ifstream inputFile;
	inputFile.open(inputFileName, std::ios::binary);
	inputFile.seekg(offset, std::ios::beg);

	if (!inputFile)
	{
		std::cerr << "Failed to open the file for reading!" << std::endl;
		exit(1);
	}

	uint64_t freq;

	for (int i = 0; i < 256; i++)
	{
		inputFile.read(reinterpret_cast<char*>(&freq), sizeof(freq));
		byteFrequency.push_back(freq);
	}

	inputFile.close();
}

void printHuffmanCode(std::unordered_map<uint8_t, std::string> huffmanCodes)
{
	for (const auto& pair : huffmanCodes)
	{
		uint8_t key = pair.first;
		const std::string& value = pair.second;
		std::cout << "Byte: " << static_cast<int>(key) << " Code: " << value << std::endl;
	}
}

uint32_t compressFile(const string& needToCompressFilename, const string& tempCodeFilename, const vector<uint64_t>& byteFrequency, std::unordered_map<uint8_t, std::string> huffmanCodes)
{

	ifstream needToCompressFile; // read from it
	ofstream tempCodeFile; // write to it

	tempCodeFile.open(tempCodeFilename, ios::binary);
	needToCompressFile.open(needToCompressFilename, ios::binary);
	 
	if (!tempCodeFile.is_open() || !needToCompressFile.is_open())
	{
		cerr << "Ошибка при открытии файлов!" << endl;
		return 0;
	}

	string encodedString = "";
	uint32_t length = 0;
	uint8_t byte;

	while (needToCompressFile.read(reinterpret_cast<char*>(&byte), sizeof(byte)))
	{
		encodedString += huffmanCodes[byte];
		while (encodedString.length() > 7)
		{
			bitset<8> bits(encodedString.substr(0, 8));
			tempCodeFile.put(static_cast<uint8_t>(bits.to_ulong()));
			encodedString.erase(0, 8);
			length += 1;
		}
	}

	// Дополните строку до 8 бит
	auto temp = encodedString.length();
	while (encodedString.length() < 8)
	{
		encodedString += '0';
	}

	bitset<8> bits(encodedString);
	tempCodeFile.put(static_cast<uint8_t>(bits.to_ulong()));
	tempCodeFile.put(static_cast<uint8_t>(temp));
	length += 2;

	tempCodeFile.close();
	needToCompressFile.close();
	return length;
}

int decompressFile(const std::string& archiveFileName, const std::string& outputFileName, const std::vector<uint64_t>& byteFrequency, std::unordered_map<uint8_t, std::string> huffmanCodes, int offset)
{
	auto maxLengthIt = std::max_element(
		huffmanCodes.begin(),
		huffmanCodes.end(),
		[](const pair<uint8_t, string>& a, const pair<uint8_t, string>& b) {
			return a.second.length() < b.second.length(); // Сравниваем длины строк
		}
	);

	unordered_map<string, uint8_t> reverseHuffmanCodes;
	for (const auto& pair : huffmanCodes)
	{
		reverseHuffmanCodes[pair.second] = pair.first;
	}

	ifstream archiveFile(archiveFileName, ios::binary);
	ofstream outputFile(outputFileName, ios::binary);

	if (!archiveFile.is_open() || !outputFile.is_open())
	{
		cerr << "Ошибка при открытии файлов!" << endl;
		return 0;
	}

	uint32_t compressedSize;
	uint32_t uncompressedSize;

	archiveFile.seekg(offset, std::ios::beg);

	archiveFile.read(reinterpret_cast<char*>(&compressedSize), sizeof(compressedSize));
	archiveFile.read(reinterpret_cast<char*>(&uncompressedSize), sizeof(uncompressedSize));

	string encodedString, temp = "";
	uint8_t byte, lastByte;
	size_t index = 0;

	for (int i = 0; i < compressedSize - 2; i++)
	{
		archiveFile.read(reinterpret_cast<char*>(&byte), sizeof(byte));
		encodedString += std::bitset<8>(byte).to_string();

		while (index < encodedString.size())
		{
			temp += encodedString[index];
			index++;
			auto it = reverseHuffmanCodes.find(temp);
			if (it != reverseHuffmanCodes.end()) 
			{
				outputFile.put(it->second);
				encodedString.erase(0, temp.size());
				temp.clear();
				index = 0;
			}
		}
	}

	archiveFile.read(reinterpret_cast<char*>(&byte), sizeof(byte));
	archiveFile.read(reinterpret_cast<char*>(&lastByte), sizeof(lastByte));

	temp = std::bitset<8>(byte).to_string();
	temp = temp.substr(0, lastByte);
	encodedString += temp;

	index = 0;
	temp.clear();
	while (encodedString.size())
	{
		temp += encodedString[index];
		index++;
		auto it = reverseHuffmanCodes.find(temp);
		if (it != reverseHuffmanCodes.end())
		{
			outputFile.put(it->second);
			encodedString.erase(0, temp.size());
			temp.clear();
			index = 0;
		}
	}

	offset = archiveFile.tellg();
	archiveFile.close();
	outputFile.close();
	return offset;
}