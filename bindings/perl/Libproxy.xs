#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include <proxy.h>



typedef struct _pxProxyFactory ProxyFactory;


void XS_pack_charPtrPtr( SV * arg, char ** array, int count) {
  int i;
  AV * avref;
  avref = (AV*)sv_2mortal((SV*)newAV());
  for (i=0; i<count; i++) {
    av_push(avref, newSVpv(array[i], strlen(array[i])));
  }
  SvSetSV( arg, newRV((SV*)avref));
}


MODULE = Net::Libproxy PACKAGE = Net::Libproxy


ProxyFactory *
proxy_factory_new()
  PREINIT:
    ProxyFactory *pf;
  CODE:
    pf = px_proxy_factory_new();
    RETVAL = pf;
  OUTPUT:
    RETVAL

char **
proxy_factory_get_proxies(pf, url)
    ProxyFactory * pf
    char * url
  PREINIT:
    char ** array;
    int count_charPtrPtr;
    int i;
  CODE:
    array = px_proxy_factory_get_proxies(pf, url);
    RETVAL = array;
    i = 0;
    while( array[i] != NULL ) {
      i++;
    }
    count_charPtrPtr = i;
  OUTPUT:
    RETVAL


MODULE = Net::Libproxy PACKAGE = Net::Libproxy::ProxyFactoryPtr

void
DESTROY(pf)
    ProxyFactory * pf
  CODE:
    printf("Net::Libproxy::DESTROY\n");
    px_proxy_factory_free(pf);


