#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include "openjpeg.h"

// Define jp2_box_jp here
static const unsigned char jp2_box_jp[] = {0x6a, 0x50, 0x20, 0x20}; /* 'jP  ' */

typedef struct {
    const uint8_t* pabyData;
    size_t         nCurPos;
    size_t         nLength;
} MemFile;

static void ErrorCallback(const char * msg, void *) {
    (void)msg;
}

static void WarningCallback(const char *, void *) {
}

static void InfoCallback(const char *, void *) {
}

static OPJ_SIZE_T ReadCallback(void* pBuffer, OPJ_SIZE_T nBytes, void *pUserData) {
    MemFile* memFile = (MemFile*)pUserData;
    if (memFile->nCurPos >= memFile->nLength) {
        return -1;
    }
    size_t nToRead = (nBytes + memFile->nCurPos > memFile->nLength)
                     ? memFile->nLength - memFile->nCurPos
                     : nBytes;
    memcpy(pBuffer, memFile->pabyData + memFile->nCurPos, nToRead);
    memFile->nCurPos += nToRead;
    return nToRead;
}

static OPJ_BOOL SeekCallback(OPJ_OFF_T nBytes, void * pUserData) {
    MemFile* memFile = (MemFile*)pUserData;
    if (nBytes < 0 || (size_t)nBytes > memFile->nLength) return OPJ_FALSE;
    memFile->nCurPos = (size_t)nBytes;
    return OPJ_TRUE;
}

static OPJ_OFF_T SkipCallback(OPJ_OFF_T nBytes, void * pUserData) {
    MemFile* memFile = (MemFile*)pUserData;
    size_t newPos = memFile->nCurPos + nBytes;
    if (newPos > memFile->nLength) {
        memFile->nCurPos = memFile->nLength;
        return -1;
    }
    memFile->nCurPos = newPos;
    return nBytes;
}

int LLVMFuzzerInitialize(int* /*argc*/, char*** argv) {
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) return 0;
    FILE* file = fopen(argv[1], "rb");
    if (!file) return 0;

    if (fseeko(file, 0, SEEK_END) != 0) return 0;
    size_t size = ftello(file);
    if (size < 8) return 0; // Require at least 8 bytes for options.
    rewind(file);

    uint8_t* buf = (uint8_t*)malloc(size);
    if (!buf) return 0;
    fread(buf, 1, size, file);
    fclose(file);

    // Parse options from the first 8 bytes of input
    uint32_t cp_reduce = buf[0] % 10; // Reduce level: 0 to 9
    uint32_t cp_layer = buf[1] % 5;  // Quality layer: 0 to 4
    uint32_t decode_width = (buf[2] | (buf[3] << 8)) % 4096; // Decode width limit
    uint32_t decode_height = (buf[4] | (buf[5] << 8)) % 4096; // Decode height limit
    uint32_t buffer_size = (buf[6] | (buf[7] << 8)) % 8192 + 1024; // Buffer size

    OPJ_CODEC_FORMAT eCodecFormat;
    if (size >= 4 + sizeof(jp2_box_jp) &&
        memcmp(buf + 4, jp2_box_jp, sizeof(jp2_box_jp)) == 0) {
        eCodecFormat = OPJ_CODEC_JP2;
    } else {
        free(buf);
        return 0;
    }

    opj_codec_t* pCodec = opj_create_decompress(eCodecFormat);
    opj_set_info_handler(pCodec, InfoCallback, NULL);
    opj_set_warning_handler(pCodec, WarningCallback, NULL);
    opj_set_error_handler(pCodec, ErrorCallback, NULL);

    opj_dparameters_t parameters;
    opj_set_default_decoder_parameters(&parameters);
    parameters.cp_reduce = cp_reduce;
    parameters.cp_layer = cp_layer;
    opj_setup_decoder(pCodec, &parameters);

    opj_stream_t *pStream = opj_stream_create(buffer_size, OPJ_TRUE);
    MemFile memFile = {buf, 0, size};
    opj_stream_set_user_data_length(pStream, size);
    opj_stream_set_read_function(pStream, ReadCallback);
    opj_stream_set_seek_function(pStream, SeekCallback);
    opj_stream_set_skip_function(pStream, SkipCallback);
    opj_stream_set_user_data(pStream, &memFile, NULL);

    opj_image_t * psImage = NULL;
    if (!opj_read_header(pStream, pCodec, &psImage)) {
        opj_destroy_codec(pCodec);
        opj_stream_destroy(pStream);
        free(buf);
        return 0;
    }

    // Limit decode area based on extracted options
    uint32_t width = psImage->x1 - psImage->x0;
    uint32_t height = psImage->y1 - psImage->y0;

    uint32_t width_to_read = (decode_width < width) ? decode_width : width;
    uint32_t height_to_read = (decode_height < height) ? decode_height : height;

    if (opj_set_decode_area(pCodec, psImage,
                            psImage->x0, psImage->y0,
                            psImage->x0 + width_to_read,
                            psImage->y0 + height_to_read)) {
        opj_decode(pCodec, pStream, psImage);
    }

    opj_end_decompress(pCodec, pStream);
    opj_stream_destroy(pStream);
    opj_destroy_codec(pCodec);
    opj_image_destroy(psImage);
    free(buf);

    return 0;
}
