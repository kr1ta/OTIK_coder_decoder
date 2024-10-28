#include <iostream>
#include <fstream>
#include <direct.h> 
#include <filesystem>
#include <cstdio>
#define HEADER_LENGTH 14
#include "huffman.h"

using namespace std;
namespace fs = std::filesystem;

const char mySignature[] = { 'K', 'r', 'i', 't', 'a', '!' };
const int signatureLength = sizeof(mySignature);

void createDirectories(const string& path);

struct FileInfo
{
	string relativePath = "";
	uint32_t size = 0;
};

void gatherFiles(const fs::path& dirPath, vector<FileInfo>& files)
{
	for (const auto& entry : fs::directory_iterator(dirPath))
	{
		if (fs::is_directory(entry.path()))
		{
			// If it's a directory, call the function recursively
			gatherFiles(entry.path(), files);
		}
		else if (fs::is_regular_file(entry.path()))
		{
			// If it's a regular file, add it to the list
			string pathString = entry.path().string();
			std::replace(pathString.begin(), pathString.end(), '\\', '/'); // Replace backslashes with forward slashes
			files.push_back({ pathString, static_cast<uint32_t>(fs::file_size(entry.path())) });
		}
	}
}

void Coder(const vector<FileInfo>& files, uint16_t compressAndInterfernce)
{
	string tempFilename = "temp";
	string archiveName = "archive.krit"; // name of archive

	ofstream tempFile;

	std::remove(tempFilename.c_str());
	std::remove(archiveName.c_str());

	tempFile.open(tempFilename, ios::binary | ios::app);

	if (!tempFile)
	{
		cerr << "Failed to open temporary file for writing." << endl;
		return;
	}

	tempFile.write(mySignature, signatureLength);
	uint16_t version = 0x0004; // Example version (4.0)
	tempFile.write(reinterpret_cast<char*>(&version), sizeof(version));
	tempFile.write(reinterpret_cast<char*>(&compressAndInterfernce), sizeof(compressAndInterfernce));

	uint16_t extraFieldLengthValue = 0;

	if (compressAndInterfernce == 1)
	{
		extraFieldLengthValue = 256 * 8; // for frequency table
	}

	uint16_t offsetFilesStart = HEADER_LENGTH + extraFieldLengthValue;
	tempFile.write(reinterpret_cast<char*>(&offsetFilesStart), sizeof(offsetFilesStart));
	tempFile.write(reinterpret_cast<char*>(&extraFieldLengthValue), sizeof(extraFieldLengthValue));
	tempFile.close();

	if (compressAndInterfernce == 0) // no compression, just copy data
	{
		for (const auto& file : files)
		{
			tempFile.open(tempFilename, ios::binary | ios::app);

			ifstream inputFile;
			inputFile.open(file.relativePath, ios::binary);
			if (!inputFile)
			{
				cerr << "Failed to open file for reading: " << file.relativePath << endl;
				continue;
			}

			inputFile.seekg(0, ios::end);
			uint32_t originalSize = inputFile.tellg();
			inputFile.seekg(0, ios::beg);

			uint8_t nameLength = static_cast<uint8_t>(file.relativePath.length());
			tempFile.write(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
			tempFile.write(file.relativePath.c_str(), nameLength);

			uint32_t compressedSize = originalSize;
			tempFile.write(reinterpret_cast<char*>(&compressedSize), sizeof(compressedSize));
			tempFile.write(reinterpret_cast<char*>(&originalSize), sizeof(originalSize));

			tempFile << inputFile.rdbuf();
			inputFile.close();
			tempFile.close();
		}
	}
	if (compressAndInterfernce == 1) // huffman compression
	{
		vector<uint64_t> byteFrequency(256, 0);
		for (const auto& file : files)
		{
			countByteFrequency(file.relativePath, byteFrequency);
		}

		writeFrequencyTable(tempFilename, byteFrequency);
		Node* root = buildHuffmanTree(byteFrequency);
		unordered_map<uint8_t, string> huffmanCodes;
		generateCodes(root, "", huffmanCodes);

		for (const auto& file : files)
		{
			tempFile.open(tempFilename, ios::binary | ios::app);

			ifstream fileToCode;
			fileToCode.open(file.relativePath, ios::binary);
			if (!fileToCode)
			{
				cerr << "Failed to open file for reading: " << file.relativePath << endl;
				continue;
			}

			fileToCode.seekg(0, ios::end);
			uint32_t originalSize = fileToCode.tellg();
			fileToCode.seekg(0, ios::beg);

			uint8_t nameLength = static_cast<uint8_t>(file.relativePath.length());
			tempFile.write(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
			tempFile.write(file.relativePath.c_str(), nameLength);

			tempFile.close();

			string filenameWhereTempCode = "wgfuiwgfuiw";
			uint32_t compressedSize = compressFile(file.relativePath, filenameWhereTempCode, byteFrequency, huffmanCodes);

			tempFile.open(tempFilename, ios::binary | ios::app);
			tempFile.write(reinterpret_cast<char*>(&compressedSize), sizeof(compressedSize));
			tempFile.write(reinterpret_cast<char*>(&originalSize), sizeof(originalSize));

			ifstream fileWhereTempCode;
			fileWhereTempCode.open(filenameWhereTempCode, ios::binary);
			tempFile << fileWhereTempCode.rdbuf();

			fileWhereTempCode.close();
			fileToCode.close();
			tempFile.close();
			remove(filenameWhereTempCode.c_str());
		}

		delete root;
		tempFile.close();
	}

	std::remove(archiveName.c_str()); // if archive exists
	if (rename(tempFilename.c_str(), archiveName.c_str()) != 0)
	{
		cerr << "Failed to rename temporary file." << endl;
		return;
	}
	
	cout << "Files successfully archived into " << archiveName << "." << endl;
}

bool Decoder(const string& inputFile)
{
	ifstream file;
	file.open(inputFile, ios::binary);

	file.seekg(0, std::ios::end);
	int fileSize = file.tellg();
	file.seekg(0, std::ios::beg);

	if (!file)
	{
		cerr << "Failed to open file for decoding." << endl;
		return false;
	}

	char signatureBuffer[signatureLength];
	file.read(signatureBuffer, signatureLength);

	if (file.gcount() != signatureLength || memcmp(signatureBuffer, mySignature, signatureLength) != 0)
	{
		cerr << "Invalid signature in the file." << endl;
		return false;
	}

	uint16_t version;
	uint16_t compressAndInterfernce;
	uint16_t offsetFilesStart;
	uint16_t extraFieldLengthValue;

	file.read(reinterpret_cast<char*>(&version), sizeof(version));
	file.read(reinterpret_cast<char*>(&compressAndInterfernce), sizeof(compressAndInterfernce));
	file.read(reinterpret_cast<char*>(&offsetFilesStart), sizeof(offsetFilesStart));
	file.read(reinterpret_cast<char*>(&extraFieldLengthValue), sizeof(extraFieldLengthValue));

	int offset = offsetFilesStart;

	if (compressAndInterfernce == 0) // no compression
	{
		while (1)
		{
			uint8_t nameLength;
			file.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
			if (file.eof()) break;

			char* filenameBuffer = new char[nameLength + 1];
			file.read(filenameBuffer, nameLength);
			filenameBuffer[nameLength] = '\0';
			string unpackedFilename(filenameBuffer);

			delete[] filenameBuffer;
			uint32_t compressedSize;
			uint32_t uncompressedSize;

			file.read(reinterpret_cast<char*>(&compressedSize), sizeof(compressedSize));
			file.read(reinterpret_cast<char*>(&uncompressedSize), sizeof(uncompressedSize));

			createDirectories(unpackedFilename);
			ofstream unpackedFile(unpackedFilename, ios::binary);

			if (!unpackedFile.is_open())
			{
				cerr << "Failed to open unpacked file " << unpackedFilename << " for writing." << endl;
				return false;
			}

			vector<char> buffer(compressedSize);
			file.read(buffer.data(), compressedSize);
			unpackedFile.write(buffer.data(), uncompressedSize);
			unpackedFile.close();
			cout << "Successfully created unpacked file: " << unpackedFilename << endl;
		}

		file.close();
		return true;
	} 
	if (compressAndInterfernce == 1) // huffman comression
	{
		std::vector<uint64_t> byteFrequency;
		readFrequencyTable(inputFile, byteFrequency, 14);
		Node* root = buildHuffmanTree(byteFrequency);

		unordered_map<uint8_t, string> huffmanCodes;
		generateCodes(root, "", huffmanCodes);

		while (1)
		{
			file.seekg(offset, std::ios::beg);

			uint8_t nameLength;
			file.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
			if (file.eof()) break;

			char* filenameBuffer = new char[nameLength + 1];
			file.read(filenameBuffer, nameLength);

			filenameBuffer[nameLength] = '\0';
			string unpackedFilename(filenameBuffer);

			delete[] filenameBuffer;

			createDirectories(unpackedFilename);
			ofstream unpackedFile(unpackedFilename, ios::binary);

			if (!unpackedFile.is_open())
			{
				cerr << "Failed to open unpacked file " << unpackedFilename << " for writing." << endl;
				return false;
			}

			offset = file.tellg();
			offset = decompressFile(inputFile, unpackedFilename, byteFrequency, huffmanCodes, offset);

			unpackedFile.close();
			cout << "Successfully created unpacked file: " << unpackedFilename << endl;
			if (offset > fileSize - 1)
				break;
		}

		file.close();
		return true;
	}
	return false;
}

bool fileExists(const string& filename)
{
	ifstream fileStream(filename.c_str());
	return fileStream.good();
}

void createDirectories(const string& path)
{
	string currentPath;

	size_t pos = 0;

	if (path.find('/') == std::string::npos)
	{
		std::ofstream file(path);
		return;
	}

	while ((pos = path.find('/', pos)) != string::npos)
	{
		currentPath = path.substr(0, pos);

		_mkdir(currentPath.c_str());

		pos++; // Move to next character after '/'
	}

	// create a file (if no dir, just file)
	currentPath = path.substr(0, path.find_last_of('/'));
	_mkdir(currentPath.c_str());
}

uint16_t askForCompress() // to choose compression method
{
	cout << "\n\Compression methods:\n";
	cout << "1) No\n";
	cout << "2) Huffman\n";
	cout << "Choose an option: ";

	int choice;
	std::cin >> choice;

	switch (choice)
	{
	case 1:
	{
		return 0;
		break;
	}
	case 2:
	{
		return 1;
		break;
	}
	case 3:
	{
		return 2;
		break;
	}
	}
}

int main()
{
	while (true)
	{
		cout << "\n\tMenu:\n";
		cout << "1) Encode specific files\n";
		cout << "2) Encode all files in a directory\n";
		cout << "3) Decode an archive\n";
		cout << "6) Exit\n";
		cout << "Choose an option: ";

		int choice;
		std::cin >> choice;

		if (choice == 6)
			break;

		switch (choice)
		{
		case 1:
		{
			int numFiles;
			cout << "Enter number of files to encode: ";
			std::cin >> numFiles;

			vector<FileInfo> files(numFiles);

			for (int i = 0; i < numFiles; ++i)
			{
				cout << "Enter the full path of the file to encode: ";
				string filepath;
				std::cin >> filepath;

				if (!fileExists(filepath))
				{
					cout << "File does not exist: " << filepath << ". Please try again.\n";
					--i;
					continue;
				}

				files[i].relativePath = filepath;
			}

			uint16_t comp = askForCompress();

			Coder(files, comp);
			break;
		}
		case 2:
		{
			string dirPath;
			cout << "Enter the full path of the directory to encode: ";
			std::cin >> dirPath;

			if (!fs::exists(dirPath) || !fs::is_directory(dirPath))
			{
				cout << "Directory does not exist: " << dirPath << ". Please try again.\n";
				break;
			}

			vector<FileInfo> files;
			gatherFiles(dirPath, files);

			if (files.empty())
			{
				cout << "No files found in the specified directory.\n";
				break;
			}

			uint16_t comp = askForCompress();

			Coder(files, comp);
			break;
		}
		case 3:
		{
			string filename;
			cout << "Enter the name of the encoded archive to decode (including extension): ";
			std::cin >> filename;

			if (fileExists(filename))
			{
				Decoder(filename);
			}
			else
			{
				cout << "Encoded archive does not exist. Please try again.\n";
			}
			break;
		}
		case 4:
		{
			int numFiles;
			cout << "Enter number of files to encode: ";
			std::cin >> numFiles;

			vector<FileInfo> files(numFiles);

			for (int i = 0; i < numFiles; ++i)
			{
				cout << "Enter the full path of the file to encode: ";
				string filepath;
				std::cin >> filepath;

				if (!fileExists(filepath))
				{
					cout << "File does not exist: " << filepath << ". Please try again.\n";
					--i;
					continue;
				}

				files[i].relativePath = filepath;
			}

			uint16_t comp = askForCompress();

			Coder(files, comp);
			break;
		}
		default:
			cout << "Invalid option. Please choose again.\n";
			break;
		}
	}
	return 0;
}
