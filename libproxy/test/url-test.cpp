#include <string>
#include <iostream>
#include <cassert>

#include "url.hpp"

using namespace libproxy;

#define try_url(l) _try_url(l,__FILE__,__LINE__)
bool _try_url (std::string link, const char *file, int line)
{
  bool rtv = true;
  try {
      url u(link);
  }
  catch (exception e) {
      std::cerr << "Can't parse '" << link << "'" 
		<< " (" << file << ":" << line << ")" 
		<< std::endl;
      rtv = false;
  }
  return rtv;
}

int main()
{
  bool rtv = true;
  
  rtv = try_url ("file:///allo") && rtv;
  rtv = try_url ("http://test.com") && rtv;
  rtv = try_url ("http://test.com/") && rtv;
  rtv = try_url ("http://test.com#") && rtv;
  rtv = try_url ("http://test.com?") && rtv;
  rtv = try_url ("http://nicolas@test.com") && rtv;
  rtv = try_url ("http://nicolas:@test.com") && rtv;
  rtv = try_url ("http://nicolas:secret@test.com") && rtv;
  rtv = try_url ("http://:secret@test.com") && rtv;
  rtv = try_url ("http+ssh://:secret@test.com") && rtv;
  
  return !rtv;
}
