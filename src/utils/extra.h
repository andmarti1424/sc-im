#include "../buffer.h"

void nofreeNULL(void *x);
char * v_name(int row, int col);       // Returns the ROW/COL cell name
char * parse_cell_name(int ignore_first_blocks, struct block * buf_in); // Parse BUF_IN to get a cell name. Skip first blocks with IGNORE_FIRST_BLOCKS
