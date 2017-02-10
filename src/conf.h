#define DEFAULT_CONFIG_DIR "/.config"
#define CONFIG_LOCATION "/scim/config"

#define COMMENT_CHAR '#'

extern struct dictionary * user_conf_d;
extern struct dictionary * predefined_conf_d;

char * config_path();
void load_config_file();
const int get_config_color(char * key, const int default_color);

void setup_user_conf();
char * get_conf_value(char * key);
char * get_conf_values(char * salida);
