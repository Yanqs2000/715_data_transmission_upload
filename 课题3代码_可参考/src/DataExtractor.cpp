#include "DataExtractor.h"
#include "CheckSum.h"

// 用于将四个字节转换为一个32位整数（小端字节序）
uint32_t bytesToUint32(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3) {
    return (b3 << 24) | (b2 << 16) | (b1 << 8) | b0;
}

// 用于将两个字节转换为一个16位整数（小端字节序）
uint16_t bytesToUint16(uint8_t b0, uint8_t b1) {
    return (b1 << 8) | b0;
}

int DataExtractor_raw(std::vector<uint8_t> frame, std::vector<std::uint32_t> &date_time)
{
    std::vector<std::string> Date_Time;
    if(frame.size() < 110)
    {
        // std::cout<< "Data receive error: frame size = " << frame.size()<< std::endl;
        LOG(INFO) << "DataExtractor:Data receive error: frame size = " << frame.size();
        return -1;
    }
    uint16_t sum = calChecksum(frame, 108);
    uint16_t Checksum = bytesToUint16(frame[108], frame[109]);
    if(sum != Checksum)
    {
        // std::cout<< "Check Sum error" << std::endl;
        LOG(INFO) << "Check Sum error";
        //return -1;
    }

    // // 提取特定字节并转换
    // uint32_t latitude = bytesToUint32(frame[6], frame[7], frame[8], frame[9]);
    // uint32_t longitude = bytesToUint32(frame[10], frame[11], frame[12], frame[13]);
    // uint16_t roll = bytesToUint16(frame[26], frame[27]);
    // uint16_t pitch = bytesToUint16(frame[28], frame[29]);
    // uint16_t heading = bytesToUint16(frame[30], frame[31]);
    // int16_t dvlVx = bytesToUint16(frame[51], frame[52]);
    // int16_t dvlVy = bytesToUint16(frame[53], frame[54]);
    // int16_t dvlVz = bytesToUint16(frame[55], frame[56]);
    // uint32_t dvlAltitude = bytesToUint32(frame[57], frame[58], frame[59], frame[60]);
    // uint8_t dvlStatus = frame[61];
    uint32_t date = bytesToUint32(frame[90], frame[91], frame[92], 0x00);
    uint32_t time = bytesToUint32(frame[93], frame[94], frame[95], 0x00);
    
    //LOG(INFO) << "raw date: " << std::to_string(frame[90]) <<std::to_string(frame[91]) << std::to_string(frame[92]);
    //LOG(INFO) << "raw time: " << std::to_string(frame[93])<<std::to_string(frame[94]) <<std::to_string( frame[95]);
    
    date_time.push_back(date);
    date_time.push_back(time);

    //LOG(INFO)<<"date_ "<<date_time[0];
    //LOG(INFO) << "time_ " << date_time[1];
     //std::cout<<"date: " <<date<<std::endl;
     //std::cout<<"time: " <<time<<std::endl;
    return 1;
}
