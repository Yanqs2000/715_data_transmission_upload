
// 然后，创建一个函数来初始化zstd压缩流，并将其与输出文件关联。以下是一个可能的实现：

#include <fstream>
#include "stream_compress.h" 
#include<bits/stdc++.h>
#include "gloghelper.h"

bool Bil2Compress(ZSTD_CCtx* cctx, std::ofstream& outFileStream, const uint16_t *pImageBuffer,size_t dataSize,
               const int &width, const int &height, const bool &raw_flag)
{
    if (raw_flag)
    {
        // 直接写入文件
        // bil_file.write(reinterpret_cast<const char *>(pImageBuffer), width * height * 2);
        if (!compressData(cctx, outFileStream, pImageBuffer, dataSize)) 
        {
            // Handling compression errors
            // std::cout<<"stream_compress.cpp: raw Compression failed " << std::endl;
            LOG(ERROR) << "stream_compress.cpp: raw Compression failed " << std::endl;
            return false;
        }
        
    }
    else
    {
        uint16_t data[width * height];
        for (int j = 0; j < width; j++)
        {
            for (int i = 0; i < height; i++)
            {
                data[i + j * height] = pImageBuffer[i * width + (width - j - 1)];
            }
        }
        // 480 bands
        // bil_file.write(reinterpret_cast<const char *>(data), width * height * 2);
        // Compress image data and write it to a file
        if (!compressData(cctx, outFileStream, data, dataSize)) {
            // Handling compression errors
            // std::cout<<"stream_compress.cpp: change Compression failed " <<std::endl;
            LOG(ERROR) << "stream_compress.cpp: change Compression failed " << std::endl;
            return false;
        }

    }
    return true;
}

bool Bil2Compress_finish(ZSTD_CCtx* cctx, std::ofstream& outFileStream, const uint16_t *pImageBuffer,size_t dataSize,
               const int &width, const int &height, const bool &raw_flag)
{

    if (raw_flag)
    {
        // 直接写入文件
        // bil_file.write(reinterpret_cast<const char *>(pImageBuffer), width * height * 2);
        finishCompression(cctx, outFileStream, pImageBuffer, dataSize);
            
    }
    else
    {
        uint16_t data[width * height];
        for (int j = 0; j < width; j++)
        {
            for (int i = 0; i < height; i++)
            {
                data[i + j * height] = pImageBuffer[i * width + (width - j - 1)];
            }
        }
        // 480 bands
        // bil_file.write(reinterpret_cast<const char *>(data), width * height * 2);
        // Compress image data and write it to a file
        finishCompression(cctx, outFileStream, pImageBuffer, dataSize);

    }

    return true;
}

// 初始化zstd压缩流
ZSTD_CCtx* initCompression(const std::string& outFile, std::ofstream& outFileStream) {
    // 打开输出文件流
    outFileStream.open(outFile, std::ios::binary);
    if (!outFileStream.is_open()) {
        // std::cerr << "Unable to open output file." << std::endl;
        LOG(ERROR) << "Unable to open output file." ;
        return nullptr;
    }

    // 创建zstd压缩上下文
    ZSTD_CCtx* cctx = ZSTD_createCCtx();
    if (cctx == nullptr) {
        // std::cerr << "Failed to create compression context." << std::endl;
        LOG(ERROR) << "Failed to create compression context.";
        return nullptr;
    }

    // 设置压缩级别
    ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, ZSTD_CLEVEL_DEFAULT);

    return cctx;
}

// 使用zstd压缩流压缩数据并写入文件
bool compressData(ZSTD_CCtx* cctx, std::ofstream& outFileStream, const void* data, size_t dataSize) 
{
    // 创建输出缓冲区
    size_t const outBufferSize = ZSTD_CStreamOutSize();
    std::unique_ptr<char[]> outBuffer(new char[outBufferSize]);

    // 开始压缩
    ZSTD_inBuffer input = { data, dataSize, 0 };
    while (input.pos < input.size) {
        ZSTD_outBuffer output = { outBuffer.get(), outBufferSize, 0 };
        size_t const remaining = ZSTD_compressStream2(cctx, &output, &input, ZSTD_e_continue);
        if (ZSTD_isError(remaining)) {
            // std::cerr << "Compression error: " << ZSTD_getErrorName(remaining) << std::endl;
            LOG(ERROR) << "Compression error: " << ZSTD_getErrorName(remaining) << std::endl;
            return false;
        }
        outFileStream.write(outBuffer.get(), output.pos);
    }

    return true;
}


// 结束压缩流，并关闭文件
void finishCompression(ZSTD_CCtx* cctx, std::ofstream& outFileStream, const void* data, size_t dataSize) 
{
    // 创建输出缓冲区
    size_t const outBufferSize = ZSTD_CStreamOutSize();
    std::unique_ptr<char[]> outBuffer(new char[outBufferSize]);

    // 结束压缩
    ZSTD_inBuffer input = { data, dataSize, 0 };
    ZSTD_outBuffer output = { outBuffer.get(), outBufferSize, 0 };
    size_t const remaining = ZSTD_compressStream2(cctx, &output, &input, ZSTD_e_end);
    if (!ZSTD_isError(remaining)) {
        outFileStream.write(outBuffer.get(), output.pos);
    } else {
        // std::cerr << "Failed to end compression stream: " << ZSTD_getErrorName(remaining) << std::endl;
        LOG(ERROR) << "Failed to end compression stream: " << ZSTD_getErrorName(remaining) ;
    }
    
    // 关闭文件流
    outFileStream.close();

    // 释放压缩上下文
    ZSTD_freeCCtx(cctx);
    // std::cout<<"++++++finish Compression+++++++"<<std::endl;
}