#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>

using namespace std;

typedef unsigned int uint32_t;
typedef unsigned short uint16_t;

struct WAVHeader
{
    char riff[4];
    uint32_t fileSize;
    char riffType[4];
    char fmt[4];
    uint32_t fmtSize;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    char data[4];
    uint32_t dataSize;
};

bool readWav(const string& fileName,WAVHeader& header,vector<short>& data){
    ifstream file(fileName,ios::binary);
    if(!file){
        cerr << "打开文件失败" << fileName <<  endl;
        return false;
    }

    file.read(header.riff,4);
    file.read(reinterpret_cast<char*>(&header.fileSize),4);
    file.read(header.riffType,4);
    if(string(header.riffType,4) != "WAVE"){
        cerr << "非有效的WAVE文件" << fileName <<endl;
        return false;
    }

    file.read(header.fmt,4);
    if(string(header.fmt,4) != "fmt ") {
        cerr << "不是有效的fmt字块" << fileName << endl;
        return false;
    }

    file.read(reinterpret_cast<char*>(&header.fmtSize),4);
    file.read(reinterpret_cast<char*>(&header.audioFormat), 2);
    file.read(reinterpret_cast<char*>(&header.numChannels), 2);
    file.read(reinterpret_cast<char*>(&header.sampleRate), 4);
    file.read(reinterpret_cast<char*>(&header.byteRate), 4);
    file.read(reinterpret_cast<char*>(&header.blockAlign), 2);
    file.read(reinterpret_cast<char*>(&header.bitsPerSample), 2);

    if(header.bitsPerSample != 16){
        cerr << "仅支持16位采样" << fileName << endl;
    }


    if(header.fmtSize > 16){
        file.seekg(header.fmtSize -16,ios::cur);
    }
 
    
    while(true){
        file.read(header.data,4);
        if(string(header.data,4) == "data"){
            break;
        }
        uint32_t chunkSize;
        file.read(reinterpret_cast<char*>(&chunkSize),4);
        file.seekg(chunkSize,ios::cur);
    }

    file.read(reinterpret_cast<char*>(&header.dataSize),4);
    data.resize(header.dataSize / sizeof(short));
    file.read(reinterpret_cast<char*>(data.data()),header.dataSize);
    return file.good();
}

bool writeWAV(const string& filename, const WAVHeader& header, const vector<short>& mixedData) {
    ofstream file(filename, ios::binary);
    if (!file) {
        cerr << "无法创建文件: " << filename << endl;
        return false;
    }

    file.write("RIFF", 4);
    file.write(reinterpret_cast<const char*>(&header.fileSize), 4);
    file.write("WAVE", 4);

    file.write("fmt ", 4);
    file.write(reinterpret_cast<const char*>(&header.fmtSize), 4);
    file.write(reinterpret_cast<const char*>(&header.audioFormat), 2);
    file.write(reinterpret_cast<const char*>(&header.numChannels), 2);
    file.write(reinterpret_cast<const char*>(&header.sampleRate), 4);
    file.write(reinterpret_cast<const char*>(&header.byteRate), 4);
    file.write(reinterpret_cast<const char*>(&header.blockAlign), 2);
    file.write(reinterpret_cast<const char*>(&header.bitsPerSample), 2);

    file.write("data", 4);
    file.write(reinterpret_cast<const char*>(&header.dataSize), 4);
    file.write(reinterpret_cast<const char*>(mixedData.data()), header.dataSize);

    return file.good();
}


bool mixWav(const WAVHeader& header1, const vector<short>& data1,const WAVHeader& header2, const vector<short>& data2,WAVHeader& outHeader, vector<short>& outData){
    if(
        header1.numChannels != header2.numChannels || 
        header1.sampleRate != header2.sampleRate
    ){
        cerr << "采样率不一致" << endl;
        return false;
    }

    outHeader = header1;
    
    uint32_t maxSamples = max(data1.size(),data2.size());
    outHeader.dataSize = maxSamples * sizeof(short);
    outHeader.fileSize = 36 + outHeader.dataSize;
    outData.resize(maxSamples);

    for(size_t i = 0;i<maxSamples;i++){
        short s1 = (i<data1.size()) ? data1[i] : 0;
        short s2 = (i<data2.size()) ? data2[i] : 0;
        int sum = s1 + s2;
        sum = max(min(sum,0x7FFF),-0x7FFF);
        outData[i] = static_cast<short>(sum);
    }

    return true;
}

int main(int argc, char*argv[]){
    string file1;
    string file2;
    string outFile;
    if(argc >= 4){
        file1 = argv[1];
        file2 = argv[2];
        outFile = argv[3];
    }else{
        return 1;
    }

    WAVHeader header1,header2,outHeader;
    vector<short> data1,data2,outData;

    if(readWav(file1,header1,data1) && readWav(file2,header2,data2)){
        mixWav(header1,data1,header2,data2,outHeader,outData);
        writeWAV(outFile,outHeader,outData);
    }
    return 0;
}