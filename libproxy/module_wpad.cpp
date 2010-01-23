#include "module_wpad.hpp"
using namespace com::googlecode::libproxy;

static const char *DEFAULT_WPAD_ORDER[] = {
	"wpad_dhcp",
	"wpad_slp",
	"wpad_dns",
	"wpad_dnsdevolution",
	NULL
};

bool wpad_module::operator<(const wpad_module& module) const {
	for (int i=0 ; DEFAULT_WPAD_ORDER[i] ; i++) {
		if (module.get_id() == DEFAULT_WPAD_ORDER[i])
			break;
		if (this->get_id() == DEFAULT_WPAD_ORDER[i])
			return true;
	}
	return false;
}
