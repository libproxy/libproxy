#include <stdio.h>
#include <strdict.h>
#include <config_file.h>

int main() {
  pxConfigFile* cf = px_config_file_new("test.d/test_libproxy_config_file.conf");
  if (!cf) {
    printf("Could not find conf file.\n.");
    return 1;
  }
  char* val1 = px_config_file_get_value(cf, "__DEFAULT__", "key1");
  char* val2 = px_config_file_get_value(cf, "SECTION", "key2");
  printf("val1 -> '%s' / val2 -> '%s'\n", val1 == NULL ? "NULL" : val1, val2 == NULL ? "NULL" : val2);
  px_config_file_free(cf);
  return 0;
}
