struct dictionary {
   int len;
   struct nlist * list;
};

struct nlist {
   char * key;
   char * val;
   struct nlist * next;
};

struct dictionary * create_dictionary();
void put(struct dictionary * d, char * k, char * v);
void destroy_dictionary(struct dictionary * d);
char * get(struct dictionary * d, char * key);
struct nlist * get_nl(struct dictionary * d, char * key);
void parse_str(struct dictionary * d, char * str);
