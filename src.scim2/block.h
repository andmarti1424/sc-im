#include "buffer.h"

int block_in_block (struct block * o, struct block * b);
int replace_block_in_block (struct block * olist, struct block * in, struct block * out);
void block_to_str(struct block * b, char * out);
