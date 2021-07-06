#include "inputadaptor.hpp"
#include <unordered_set>
#include <fstream>
#include <iostream>
#include <IPv4Layer.h>
#include <TcpLayer.h>
#include <UdpLayer.h>
#include <Packet.h>
#include <PcapFileDevice.h>
#include <arpa/inet.h>


//Read one pcap file, only read 5-tuple
InputAdaptor::InputAdaptor(std::string dir, std::string filename, uint64_t buffersize) {
    data = (adaptor_t*)calloc(1, sizeof(adaptor_t));
    data->databuffer = (unsigned char*)calloc(buffersize, sizeof(unsigned char));
    data->ptr = data->databuffer;
    data->cnt = 0;
    data->cur = 0;
    //Read pcap file
    std::string path = dir+filename;
    pcpp::PcapFileReaderDevice reader(path.c_str());
    if (!reader.open()) {
        std::cout << "[Error] Fail to open pcap file" << std::endl;
        exit(-1);
    }
    unsigned char* p = data->databuffer;
    pcpp::RawPacket rawPacket;
    while (reader.getNextPacket(rawPacket)) {
        pcpp::Packet parsedPacket(&rawPacket);
        pcpp::IPv4Layer* ipLayer = parsedPacket.getLayerOfType<pcpp::IPv4Layer>();
        if (p+sizeof(tuple_t) < data->databuffer + buffersize) {
            if (!parsedPacket.isPacketOfType(pcpp::IPv4)) continue;
            uint16_t size = (uint16_t)ntohs(ipLayer->getIPv4Header()->totalLength);
            uint32_t ipsrc = (uint32_t)ntohl(ipLayer->getIPv4Header()->ipSrc);
            uint32_t ipdst = (uint32_t)ntohl(ipLayer->getIPv4Header()->ipDst);
            memcpy(p, &ipsrc, sizeof(uint32_t));
            memcpy(p+sizeof(uint32_t), &ipdst, sizeof(uint32_t));
            memcpy(p+sizeof(uint32_t)*2, &(size), sizeof(uint16_t));
            p += sizeof(uint32_t)*2+sizeof(uint16_t);
            data->cnt++;
        }  else {
            std::cout << "[Error] not enough memory in inputadaptor" << std::endl;
            break;
        }
    }
    std::cout << "[Message] Read " << data->cnt << " items" << std::endl;
    reader.close();
}




InputAdaptor::~InputAdaptor() {
    free(data->databuffer);
    free(data);
}


//note that t.key is (dst, src)
int InputAdaptor::GetNext(tuple_t* t) {
    if (data->cur > data->cnt) {
        return -1;
    }
    uint32_t srcip = *((uint32_t*)data->ptr);
    uint32_t dstip = *((uint32_t*)(data->ptr+4));
    uint64_t key = dstip;
    key = (key << 32) | srcip;
    t->key = key;
    t->size = *((uint16_t*)(data->ptr+8));
    data->cur++;
    data->ptr += 10;
    return 1;
}


void InputAdaptor::Reset() {
    data->cur = 0;
    data->ptr = data->databuffer;
}

uint64_t InputAdaptor::GetDataSize() {
    return data->cnt;
}

unsigned char* InputAdaptor::GetPointer() {
    return data->databuffer;
}

