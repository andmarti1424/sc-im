extern struct dictionary * user_conf_d;
extern struct dictionary * predefined_conf_d;

void store_default_config_values();
char * get_conf_value(char * key);
char * get_conf_values(char * salida);