#include "xls.h"

int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    xls_error_t error;
    xlsWorkBook *work_book = xls_open_buffer(Data, Size, NULL, &error);
    
    if (work_book) {
        // 先解析整个工作簿
        xls_error_t parse_error = xls_parseWorkBook(work_book);
        
        if (parse_error == LIBXLS_OK) {
            // 如果工作簿解析成功，继续解析每个工作表
            for (int i = 0; i < work_book->sheets.count; i++) {
                xlsWorkSheet *work_sheet = xls_getWorkSheet(work_book, i);
                if (work_sheet) {
                    xls_parseWorkSheet(work_sheet);
                    xls_close_WS(work_sheet);
                }
            }
        }
        
        xls_close_WB(work_book);
    }
    
    return 0;
}
