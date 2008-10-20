/*
   Copyright header
   Demo pogram using libcurl to fetch the data, injecting proxy
   information received from libproxy.
*/

#include "proxy.h"
#include "curl/curl.h"
#include <string.h>
#include <stdlib.h>

int main(int argc, char * argv[]) {
  CURL *curl;
  CURLcode result;

  /* Check if we have a parameter passed, otherwise bail out... I need one */
  if (argc < 2)
  {
    printf("libcurl / libproxy example implementation\n");
    printf("=========================================\n");
    printf("The program has to be started with one parameter\n");
    printf("Defining the URL that should be downloaded\n\n");
    printf("Example: %s http://code.google.com/p/libproxy/\n", argv[0]);
    return -1;
  }

  /* Initializing curl... has to happen exactly once in the program */
  curl_global_init( CURL_GLOBAL_ALL );
  /* Initializing libproxy */
  pxProxyFactory *pf = px_proxy_factory_new();

  /* Using curl to get the address defined in argv[1] */
  curl = curl_easy_init();
  if (curl)
  {
    curl_easy_setopt(curl, CURLOPT_URL, argv[1]);
    char **proxies = px_proxy_factory_get_proxies(pf, argv[1]);
    for (int i=0;proxies[i];i++)
    {
      curl_easy_setopt(curl, CURLOPT_PROXY, proxies[i]);
      result = curl_easy_perform(curl);
      if (result == 0 ) continue;
    }
    // Free the proxy list
    for (int i=0 ; proxies[i] ; i++)
      free(proxies[i]);
    free(proxies);
    curl_easy_cleanup(curl);
 }

  /* let's clean up again */
  px_proxy_factory_free(pf);
  curl_global_cleanup();
  return 0;
}

