#include <iostream>
#include <cstdint>
#include <vector>
#include <thread>

// 用于将四个字节转换为一个32位整数（小端字节序）
uint32_t bytesToUint32(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3);

// 用于将两个字节转换为一个16位整数（小端字节序）
uint16_t bytesToUint16(uint8_t b0, uint8_t b1);

int DataExtractor_raw(std::vector<uint8_t> &frame, std::vector<uint8_t> &date_time);

void get_filename_from_time(char *filename, size_t len);