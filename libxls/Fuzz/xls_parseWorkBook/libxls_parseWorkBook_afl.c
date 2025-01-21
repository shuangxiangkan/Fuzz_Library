#include "xls.h"


int main(int argc, char** argv) {
  FILE* file;
  file = fopen(argv[1], "r");
  if (!file) return 0;
  if (fseeko(file, 0, SEEK_END) != 0) {
    fprintf(stderr, "fseeko error!\n");
    return 0;
  }
  size_t size = ftello(file);
  // printf("size is %zu\n", size);
  uint8_t* data;
  if (fseeko(file, 0, SEEK_SET) != 0) {
    fprintf(stderr, "fseeko error!\n");
    return 0;
  }
  data = (uint8_t*)malloc(sizeof(char) * size + 1);
  size_t fread_len = fread(data, size, 1, file);
  // printf("fread_len is %zu\n", fread_len);
  
    xls_error_t error;
    xlsWorkBook *work_book = xls_open_buffer(data, size, NULL, &error);
    
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
