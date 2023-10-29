#include "stdafx.h"
#include "PacketEncryptor.h"

#include "CSerializeBuffer.h"

void PacketEncryptor::Encode(CSerializeBuffer& buffer)
{
    //조건에 맞다면...
    encodeBuffer(buffer);
}

void PacketEncryptor::Decode(CSerializeBuffer& buffer)
{
    //조건에 맞다면
    //decode는 한 스레드에서 처리하니까 그냥 해도 될 것 같음. 
    decodeBuffer(buffer);
}

void PacketEncryptor::encodeBuffer(CSerializeBuffer& buffer)
{
    Header* head = (Header*)buffer.GetBufferPtr();
    char* payLoad = (char*)(head + 1);
    int payLoadLen = head->len - sizeof(Header);

    head->checkSum = 0;
    char* checksumIndex = payLoad;
    for (int i = 0; i < payLoadLen; i++)
    {
        head->checkSum += *payLoad;
        checksumIndex++;
    }

    char* encodeData = payLoad - 1;
    int encodeLen = payLoadLen + 1;


    char p = 0;
    char e = 0;

    for (size_t i = 1; i <= encodeLen; i++)
    {
        p = (*encodeData) ^ (p + head->randomKey + i);
        e = p ^ (e + _staticKey + i);
        *encodeData = e;
        encodeData++;
    }
}

void PacketEncryptor::decodeBuffer(CSerializeBuffer& buffer)
{
    Header* head = (Header*)buffer.GetBufferPtr();
    char* payLoad = (char*)(head + 1);
    int payLoadLen = head->len - sizeof(Header);


    unsigned char* decodeData = &head->checkSum;
    int decodeLen = payLoadLen + 1;

    char p = 0;
    char e = 0;
    char oldP = 0;
    for (size_t i = 1; i <= decodeLen; i++)
    {
        p = (*decodeData) ^ (e + _staticKey + i);
        e = *decodeData;
        *decodeData = p ^ (oldP + head->randomKey + i);
        oldP = p;
        decodeData++;
    }
}
