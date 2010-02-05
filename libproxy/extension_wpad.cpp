#include "extension_wpad.hpp"
using namespace com::googlecode::libproxy;

#include <cstring>

static const char *DEFAULT_WPAD_ORDER[] = {
	"dhcp",
	"slp",
	"dns_srv",
	"dns_txt",
	"dns_alias",
	NULL
};

bool wpad_extension::operator<(const wpad_extension& other) const {
	for (int i=0 ; DEFAULT_WPAD_ORDER[i] ; i++) {
		if (strstr(other.base_type(), DEFAULT_WPAD_ORDER[i]))
			break;
		if (strstr(this->base_type(), DEFAULT_WPAD_ORDER[i]))
			return true;
	}
	return false;
}
