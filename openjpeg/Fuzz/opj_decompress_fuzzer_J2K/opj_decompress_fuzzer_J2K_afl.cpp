#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>

#include "openjpeg.h"

typedef struct {
    const uint8_t* pabyData;
    size_t         nCurPos;
    size_t         nLength;
} MemFile;

static void ErrorCallback(const char * msg, void *)
{
    // 保留错误回调以记录潜在问题
    fprintf(stderr, "OpenJPEG Error: %s\n", msg);
}

static void WarningCallback(const char * msg, void *)
{
    fprintf(stderr, "OpenJPEG Warning: %s\n", msg);
}

static void InfoCallback(const char * msg, void *)
{
    // 如果需要可以取消注释记录信息
    // fprintf(stderr, "OpenJPEG Info: %s\n", msg);
}

static OPJ_SIZE_T ReadCallback(void* pBuffer, OPJ_SIZE_T nBytes, void *pUserData)
{
    MemFile* memFile = (MemFile*)pUserData;
    if (memFile->nCurPos >= memFile->nLength) {
        return -1;
    }
    if (memFile->nCurPos + nBytes >= memFile->nLength) {
        size_t nToRead = memFile->nLength - memFile->nCurPos;
        memcpy(pBuffer, memFile->pabyData + memFile->nCurPos, nToRead);
        memFile->nCurPos = memFile->nLength;
        return nToRead;
    }
    if (nBytes == 0) {
        return -1;
    }
    memcpy(pBuffer, memFile->pabyData + memFile->nCurPos, nBytes);
    memFile->nCurPos += nBytes;
    return nBytes;
}

static OPJ_BOOL SeekCallback(OPJ_OFF_T nBytes, void * pUserData)
{
    MemFile* memFile = (MemFile*)pUserData;
    memFile->nCurPos = nBytes;
    return OPJ_TRUE;
}

static OPJ_OFF_T SkipCallback(OPJ_OFF_T nBytes, void * pUserData)
{
    MemFile* memFile = (MemFile*)pUserData;
    memFile->nCurPos += nBytes;
    return nBytes;
}

// 动态确定编解码器格式的函数
OPJ_CODEC_FORMAT determine_codec_format(const uint8_t *data, size_t size) 
{
    // 如果输入足够大，尝试根据文件头判断
    if (size >= 2) {
        // J2K格式标识
        if (memcmp(data, "\xff\x4f", 2) == 0) {
            return OPJ_CODEC_J2K;
        }
        
        // JP2格式标识
        if (size >= 4 && memcmp(data, "\x6a\x50\x20\x20", 4) == 0) {
            return OPJ_CODEC_JP2;
        }
    }

    // 如果无法确定，默认尝试J2K解码
    return OPJ_CODEC_J2K;
}

// 动态配置解码器参数的函数
void configure_decoder_params(opj_dparameters_t* parameters, const uint8_t* data, size_t size) 
{
    // 首先重置为默认参数
    opj_set_default_decoder_parameters(parameters);

    if (size > 10) {
        // 降低解析精度 (0-4)
        parameters->cp_reduce = data[5] % 5;
        
        // 解码层数 (0-2)
        parameters->cp_layer = data[6] % 3;

        // 动态调整解码区域参数
        parameters->nb_tile_to_decode = (data[7] % 5);

        // 颜色空间处理：随机选择是否设置标志
        parameters->flags = (data[9] % 2) ? 1 : 0;
    }
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) 
{
    if (size < 10) return 0;  // 输入太小直接返回

    // 动态确定编解码器格式
    OPJ_CODEC_FORMAT eCodecFormat = determine_codec_format(data, size);

    opj_codec_t* pCodec = opj_create_decompress(eCodecFormat);
    opj_set_info_handler(pCodec, InfoCallback, NULL);
    opj_set_warning_handler(pCodec, WarningCallback, NULL);
    opj_set_error_handler(pCodec, ErrorCallback, NULL);

    opj_dparameters_t parameters;
    // 根据输入文件动态配置参数
    configure_decoder_params(&parameters, data, size);

    opj_setup_decoder(pCodec, &parameters);

    opj_stream_t *pStream = opj_stream_create(1024, OPJ_TRUE);
    MemFile memFile;
    memFile.pabyData = data;
    memFile.nLength = size;
    memFile.nCurPos = 0;
    opj_stream_set_user_data_length(pStream, size);
    opj_stream_set_read_function(pStream, ReadCallback);
    opj_stream_set_seek_function(pStream, SeekCallback);
    opj_stream_set_skip_function(pStream, SkipCallback);
    opj_stream_set_user_data(pStream, &memFile, NULL);

    opj_image_t * psImage = NULL;
    if (!opj_read_header(pStream, pCodec, &psImage)) {
        opj_destroy_codec(pCodec);
        opj_stream_destroy(pStream);
        return 0;
    }

    // 限制解码区域大小
    OPJ_UINT32 width = psImage->x1 - psImage->x0;
    OPJ_UINT32 height = psImage->y1 - psImage->y0;
    OPJ_UINT32 width_to_read = (width > 1024) ? 1024 : width;
    OPJ_UINT32 height_to_read = (height > 1024) ? 1024 : height;

    if (opj_set_decode_area(pCodec, psImage,
                            psImage->x0, psImage->y0,
                            psImage->x0 + width_to_read,
                            psImage->y0 + height_to_read)) {
        opj_decode(pCodec, pStream, psImage);
    }

    // 清理资源
    opj_end_decompress(pCodec, pStream);
    opj_stream_destroy(pStream);
    opj_destroy_codec(pCodec);
    opj_image_destroy(psImage);

    return 0;
}

// 为传统的main函数保留入口，以支持独立编译
int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    FILE* file = fopen(argv[1], "rb");
    if (!file) return 0;

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    uint8_t* data = (uint8_t*)malloc(size);
    size_t fread_len = fread(data, 1, size, file);
    fclose(file);

    int result = LLVMFuzzerTestOneInput(data, fread_len);
    free(data);

    return result;
}