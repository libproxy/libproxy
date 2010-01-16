#include <stdio.h>

#include "../libproxy/strdict.h"
#include "../libproxy/config_file.h"

int main(int argc, char **argv) {
  if (argc != 4) {
    printf("You started with %d parameters.\n", argc);
    printf("Wrong invocation. Start with 3 parameters:\n");
    printf("1. path to config file\n2. Section name in conf file.\n3. Keyname to read.\n");
    printf("iniparse will return \"key = value\" if found\n.");
    return 2;
  }
  pxConfigFile* cf = px_config_file_new(argv[1]);
  if (!cf) {
    printf("Could not find conf file.\n.");
    return 1;
  }
  char* val1 = px_config_file_get_value(cf, argv[2], argv[3]);
  printf("%s = %s", argv[3] == NULL ? "NULL" : argv[3], val1 == NULL ? "NULL" : val1);
  px_config_file_free(cf);
  return 0;
}
