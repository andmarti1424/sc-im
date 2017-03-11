

#define TRG_READ	1
#define TRG_WRITE	2
#define TRG_LUA		4
#define TRG_SH		8
#define TRG_C		16	



struct trigger {
	int flag;	/* Read + Write + interface */
	char *file;
	char *function;
	};



