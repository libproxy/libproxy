#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include <proxy.h>


void XS_pack_charPtrPtr( SV * arg, char ** array, int count) {
  int i;
  AV * avref;
  avref = (AV*)sv_2mortal((SV*)newAV());
  for (i=0; i<count; i++) {
    av_push(avref, newSVpv(array[i], strlen(array[i])));
  }
  SvSetSV( arg, newRV((SV*)avref));
}

/* http://www.perlmonks.org/?node_id=680842 */
static char **
XS_unpack_charPtrPtr (SV *arg) {
  char **ret;
  AV *av;
  I32 i;

  if (!arg || !SvOK (arg) || !SvROK (arg) || SvTYPE (SvRV (arg)) != SVt_PVAV)
    croak ("array reference expected");

  av = (AV *)SvRV (arg);
  ret = malloc ((av_len (av) + 1 + 1) * sizeof (char *));
  if (!ret)
    croak ("malloc failed");

  for (i = 0; i <= av_len (av); i++) {
    SV **elem = av_fetch (av, i, 0);

    if (!elem || !*elem) {
      free (ret);
      croak ("missing element in list");
    }

    ret[i] = SvPV_nolen (*elem);
  }

  ret[i] = NULL;

  return ret;
}


MODULE = Net::Libproxy PACKAGE = Net::Libproxy

PROTOTYPES: DISABLE

pxProxyFactory *
proxy_factory_new()
  PREINIT:
    pxProxyFactory *pf;
  CODE:
    pf = px_proxy_factory_new();
    RETVAL = pf;
  OUTPUT:
    RETVAL

char **
proxy_factory_get_proxies(pf, url)
    pxProxyFactory * pf
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

void
proxy_factory_free_proxies(proxies)
    char ** proxies
  CODE:
    px_proxy_factory_free_proxies(proxies);

MODULE = Net::Libproxy PACKAGE = Net::Libproxy::ProxyFactoryPtr

void
DESTROY(pf)
    pxProxyFactory * pf
  CODE:
    printf("Net::Libproxy::DESTROY\n");
    px_proxy_factory_free(pf);


