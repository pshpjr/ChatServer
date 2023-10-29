#pragma once

class PacketEncryptor
{
    PacketEncryptor(char staticKey) : _staticKey(staticKey) {  }

    struct Header {
        char code;
        short len;
        char randomKey;
        unsigned char checkSum;
    };

    void Encode(CSerializeBuffer& buffer);
    void Decode(CSerializeBuffer& buffer);

private:
    void encodeBuffer(CSerializeBuffer& buffer);
    void decodeBuffer(CSerializeBuffer& buffer);


private:
    const char _staticKey;



};
