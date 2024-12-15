// #pragma once
#ifndef STREAM_COMPRESS_H
#define STREAM_COMPRESS_H
#endif
#include<zstd.h>
#include <fstream>
#include <filesystem>
#include "gloghelper.h"
// 初始化zstd压缩流
ZSTD_CCtx* initCompression(const std::string& outFile, std::ofstream& outFileStream);

// 使用zstd压缩流压缩数据并写入文件
bool compressData(ZSTD_CCtx* cctx, std::ofstream& outFileStream, const void* data, size_t dataSize);

// 结束压缩流，并关闭文件
void finishCompression(ZSTD_CCtx* cctx, std::ofstream& outFileStream, const void* data, size_t dataSize);

bool Bil2Compress(ZSTD_CCtx* cctx, std::ofstream& outFileStream, const uint16_t *pImageBuffer,size_t dataSize,
               const int &width, const int &height, const bool &raw_flag);

bool Bil2Compress_finish(ZSTD_CCtx* cctx, std::ofstream& outFileStream, const uint16_t *pImageBuffer,size_t dataSize,
               const int &width, const int &height, const bool &raw_flag);

