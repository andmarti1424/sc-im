#include "../buffer.h"

void nofreeNULL(void *x);
char * v_name(int row, int col);       // devuelve el nombre de una celda, a partir del numero de row y col
char * parse_cell_name(int ignore_first_blocks, struct block * buf_in); // funcion que parsea de un buf el nombre de una celda
