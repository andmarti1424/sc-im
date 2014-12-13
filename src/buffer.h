// Block of buffer
struct block {
  int value;
  struct block * pnext;
}; 

struct block * create_buf();
void addto_buf(struct block * buf, int d);
void copybuffer(struct block * origen, struct block * destino);
void del_buf (struct block * buf, int pos);
void flush_buf (struct block * buf);
void erase_buf (struct block * buf);
int get_bufsize(struct block * buf);
int get_pbuflen(struct block * buf);
int get_bufval(struct block * buf, int d);
int find_val(struct block * buf, int value);
struct block * dequeue (struct block * buf);
