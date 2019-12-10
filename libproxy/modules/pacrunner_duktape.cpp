
#include "../extension_pacrunner.hpp"
#include "pacutils.h"

#include "duktape.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#endif

using namespace libproxy;

static const char* pacUtils =
"function dnsDomainIs(host, domain) {\n"
"    return (host.length >= domain.length &&\n"
"            host.substring(host.length - domain.length) == domain);\n"
"}\n"

"function dnsDomainLevels(host) {\n"
"    return host.split('.').length-1;\n"
"}\n"

"function convert_addr(ipchars) {\n"
"    var bytes = ipchars.split('.');\n"
"    var result = ((bytes[0] & 0xff) << 24) |\n"
"                 ((bytes[1] & 0xff) << 16) |\n"
"                 ((bytes[2] & 0xff) <<  8) |\n"
"                  (bytes[3] & 0xff);\n"
"    return result;\n"
"}\n"

"function isInNet(ipaddr, pattern, maskstr) {\n"
"    var test = /^(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})$/(ipaddr);\n"
"    if (test == null) {\n"
"        ipaddr = dnsResolve(ipaddr);\n"
"        if (ipaddr == null)\n"
"            return false;\n"
"    } else if (test[1] > 255 || test[2] > 255 || \n"
"               test[3] > 255 || test[4] > 255) {\n"
"        return false;    // not an IP address\n"
"    }\n"
"    var host = convert_addr(ipaddr);\n"
"    var pat  = convert_addr(pattern);\n"
"    var mask = convert_addr(maskstr);\n"
"    return ((host & mask) == (pat & mask));\n"
"    \n"
"}\n"

"function convert_addr6(ipchars) {\n"
"    ipchars = ipchars.replace(/(^:|:$)/, '');\n"
"    var fields = ipchars.split(':');\n"
"    var diff = 8 - fields.length;\n"
"    for (var i = 0; i < fields.length; i++) {\n"
"        if (fields[i] == '') {\n"
"            fields[i] = '0';\n"
"            // inject 'diff' number of '0' elements here.\n"
"            for (var j = 0; j < diff; j++) {\n"
"                fields.splice(i++, 0, '0');\n"
"            }\n"
"            break;\n"
"        }\n"
"    }\n"
"    var result = [];\n"
"    for (var i = 0; i < fields.length; i++) {\n"
"        result.push(parseInt(fields[i], 16));\n"
"    }\n"
"    return result;\n"
"}\n"

"function isInNetEx6(ipaddr, prefix, prefix_len) {\n"
"    if (prefix_len > 128) {\n"
"        return false;\n"
"    }\n"
"    prefix = convert_addr6(prefix);\n"
"    ip = convert_addr6(ipaddr);\n"
"    // Prefix match strategy:\n"
"    //   Compare only prefix length bits between 'ipaddr' and 'prefix'\n"
"    //   Match in the batches of 16-bit fields \n"
"    prefix_rem = prefix_len % 16;\n"
"    prefix_nfields = (prefix_len - prefix_rem) / 16;\n"
"\n"
"    for (var i = 0; i < prefix_nfields; i++) {\n"
"        if (ip[i] != prefix[i]) {\n"
"            return false;\n"
"        }\n"
"    }\n"
"    if (prefix_rem > 0) {\n"
"        // Compare remaining bits\n"
"        prefix_bits = prefix[prefix_nfields] >> (16 - prefix_rem);\n"
"        ip_bits = ip[prefix_nfields] >> (16 - prefix_rem);\n"
"        if (ip_bits != prefix_bits) {\n"
"            return false;\n"
"        }\n"
"    }\n"
"    return true;\n"
"}\n"

"function isInNetEx4(ipaddr, prefix, prefix_len) {\n"
"    if (prefix_len > 32) {\n"
"        return false;\n"
"    }\n"
"    var netmask = [];\n"
"    for (var i = 1; i < 5; i++) {\n"
"        var shift_len = 8 * i - prefix_len;\n"
"        if (shift_len <= 0) {\n"
"            netmask.push(255)\n"
"        } else {\n"
"            netmask.push((0xff >> shift_len) << shift_len);\n"
"        }\n"
"    }\n"
"    return isInNet(ipaddr, prefix, netmask.join('.'));\n"
"}\n"

"function isInNetEx(ipaddr, prefix) {\n"
"    prefix_a = prefix.split('/');\n"
"    if (prefix_a.length != 2) {\n"
"        return false;\n"
"    }\n"
"    var test = /^\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}$/.test(ipaddr);\n"
"    if (!test) {\n"
"        return isInNetEx6(ipaddr, prefix_a[0], prefix_a[1]);\n"
"    } else {\n"
"        return isInNetEx4(ipaddr, prefix_a[0], prefix_a[1]);\n"
"    }\n"
"}\n"

"function isPlainHostName(host) {\n"
"    return (host.search('\\\\.') == -1);\n"
"}\n"

"function isResolvable(host) {\n"
"    var ip = dnsResolve(host);\n"
"    return (ip != null);\n"
"}\n"

"if (typeof(dnsResolveEx) == \"function\") {\n"
"function isResolvableEx(host) {\n"
"    var ip = dnsResolveEx(host);\n"
"    return (ip != null);\n"
"}\n"
"}\n"

"function localHostOrDomainIs(host, hostdom) {\n"
"    return (host == hostdom) ||\n"
"           (hostdom.lastIndexOf(host + '.', 0) == 0);\n"
"}\n"

"function shExpMatch(url, pattern) {\n"
"   pattern = pattern.replace(/\\./g, '\\\\.');\n"
"   pattern = pattern.replace(/\\*/g, '.*');\n"
"   pattern = pattern.replace(/\\?/g, '.');\n"
"   var newRe = new RegExp('^'+pattern+'$');\n"
"   return newRe.test(url);\n"
"}\n"

"var wdays = {SUN: 0, MON: 1, TUE: 2, WED: 3, THU: 4, FRI: 5, SAT: 6};\n"

"var months = {JAN: 0, FEB: 1, MAR: 2, APR: 3, MAY: 4, JUN: 5, JUL: 6, AUG: 7, SEP: 8, OCT: 9, NOV: 10, DEC: 11};\n"

"function weekdayRange() {\n"
"    function getDay(weekday) {\n"
"        if (weekday in wdays) {\n"
"            return wdays[weekday];\n"
"        }\n"
"        return -1;\n"
"    }\n"
"    var date = new Date();\n"
"    var argc = arguments.length;\n"
"    var wday;\n"
"    if (argc < 1)\n"
"        return false;\n"
"    if (arguments[argc - 1] == 'GMT') {\n"
"        argc--;\n"
"        wday = date.getUTCDay();\n"
"    } else {\n"
"        wday = date.getDay();\n"
"    }\n"
"    var wd1 = getDay(arguments[0]);\n"
"    var wd2 = (argc == 2) ? getDay(arguments[1]) : wd1;\n"
"    return (wd1 == -1 || wd2 == -1) ? false\n"
"                                    : (wd1 <= wday && wday <= wd2);\n"
"}\n"

"function dateRange() {\n"
"    function getMonth(name) {\n"
"        if (name in months) {\n"
"            return months[name];\n"
"        }\n"
"        return -1;\n"
"    }\n"
"    var date = new Date();\n"
"    var argc = arguments.length;\n"
"    if (argc < 1) {\n"
"        return false;\n"
"    }\n"
"    var isGMT = (arguments[argc - 1] == 'GMT');\n"
"\n"
"    if (isGMT) {\n"
"        argc--;\n"
"    }\n"
"    // function will work even without explict handling of this case\n"
"    if (argc == 1) {\n"
"        var tmp = parseInt(arguments[0]);\n"
"        if (isNaN(tmp)) {\n"
"            return ((isGMT ? date.getUTCMonth() : date.getMonth()) ==\n"
"getMonth(arguments[0]));\n"
"        } else if (tmp < 32) {\n"
"            return ((isGMT ? date.getUTCDate() : date.getDate()) == tmp);\n"
"        } else { \n"
"            return ((isGMT ? date.getUTCFullYear() : date.getFullYear()) ==\n"
"tmp);\n"
"        }\n"
"    }\n"
"    var year = date.getFullYear();\n"
"    var date1, date2;\n"
"    date1 = new Date(year,  0,  1,  0,  0,  0);\n"
"    date2 = new Date(year, 11, 31, 23, 59, 59);\n"
"    var adjustMonth = false;\n"
"    for (var i = 0; i < (argc >> 1); i++) {\n"
"        var tmp = parseInt(arguments[i]);\n"
"        if (isNaN(tmp)) {\n"
"            var mon = getMonth(arguments[i]);\n"
"            date1.setMonth(mon);\n"
"        } else if (tmp < 32) {\n"
"            adjustMonth = (argc <= 2);\n"
"            date1.setDate(tmp);\n"
"        } else {\n"
"            date1.setFullYear(tmp);\n"
"        }\n"
"    }\n"
"    for (var i = (argc >> 1); i < argc; i++) {\n"
"        var tmp = parseInt(arguments[i]);\n"
"        if (isNaN(tmp)) {\n"
"            var mon = getMonth(arguments[i]);\n"
"            date2.setMonth(mon);\n"
"        } else if (tmp < 32) {\n"
"            date2.setDate(tmp);\n"
"        } else {\n"
"            date2.setFullYear(tmp);\n"
"        }\n"
"    }\n"
"    if (adjustMonth) {\n"
"        date1.setMonth(date.getMonth());\n"
"        date2.setMonth(date.getMonth());\n"
"    }\n"
"    if (isGMT) {\n"
"    var tmp = date;\n"
"        tmp.setFullYear(date.getUTCFullYear());\n"
"        tmp.setMonth(date.getUTCMonth());\n"
"        tmp.setDate(date.getUTCDate());\n"
"        tmp.setHours(date.getUTCHours());\n"
"        tmp.setMinutes(date.getUTCMinutes());\n"
"        tmp.setSeconds(date.getUTCSeconds());\n"
"        date = tmp;\n"
"    }\n"
"    return ((date1 <= date) && (date <= date2));\n"
"}\n"

"function timeRange() {\n"
"    var argc = arguments.length;\n"
"    var date = new Date();\n"
"    var isGMT= false;\n"
"\n"
"    if (argc < 1) {\n"
"        return false;\n"
"    }\n"
"    if (arguments[argc - 1] == 'GMT') {\n"
"        isGMT = true;\n"
"        argc--;\n"
"    }\n"
"\n"
"    var hour = isGMT ? date.getUTCHours() : date.getHours();\n"
"    var date1, date2;\n"
"    date1 = new Date();\n"
"    date2 = new Date();\n"
"\n"
"    if (argc == 1) {\n"
"        return (hour == arguments[0]);\n"
"    } else if (argc == 2) {\n"
"        return ((arguments[0] <= hour) && (hour <= arguments[1]));\n"
"    } else {\n"
"        switch (argc) {\n"
"        case 6:\n"
"            date1.setSeconds(arguments[2]);\n"
"            date2.setSeconds(arguments[5]);\n"
"        case 4:\n"
"            var middle = argc >> 1;\n"
"            date1.setHours(arguments[0]);\n"
"            date1.setMinutes(arguments[1]);\n"
"            date2.setHours(arguments[middle]);\n"
"            date2.setMinutes(arguments[middle + 1]);\n"
"            if (middle == 2) {\n"
"                date2.setSeconds(59);\n"
"            }\n"
"            break;\n"
"        default:\n"
"          throw 'timeRange: bad number of arguments'\n"
"        }\n"
"    }\n"
"\n"
"    if (isGMT) {\n"
"        date.setFullYear(date.getUTCFullYear());\n"
"        date.setMonth(date.getUTCMonth());\n"
"        date.setDate(date.getUTCDate());\n"
"        date.setHours(date.getUTCHours());\n"
"        date.setMinutes(date.getUTCMinutes());\n"
"        date.setSeconds(date.getUTCSeconds());\n"
"    }\n"
"    return ((date1 <= date) && (date <= date2));\n"
"}\n"

"function findProxyForURL(url, host) {\n"
"    if (typeof FindProxyForURLEx == 'function') {\n"
"        return FindProxyForURLEx(url, host);\n"
"    } else {\n"
"        return FindProxyForURL(url, host);\n"
"    }\n"
"}\n";


// DNS Resolve function; used by other routines.
// This function is used by dnsResolve, dnsResolveEx, myIpAddress,
// myIpAddressEx.
static int
resolve_host(const char* hostname, char* ipaddr_list, int max_results,
    int req_ai_family)
{
    struct addrinfo hints;
    struct addrinfo* result;
    struct addrinfo* ai;
    char ipaddr[INET6_ADDRSTRLEN];
    int error;

    // Truncate ipaddr_list to an empty string.
    ipaddr_list[0] = '\0';

#ifdef _WIN32
    // On windows, we need to initialize the winsock dll first.
    WSADATA WsaData;
    WSAStartup(MAKEWORD(2, 0), &WsaData);
#endif

    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_family = req_ai_family;
    hints.ai_socktype = SOCK_STREAM;

    error = getaddrinfo(hostname, NULL, &hints, &result);
    if (error) return error;
    int i = 0;
    for (ai = result; ai != NULL && i < max_results; ai = ai->ai_next, i++) {
        getnameinfo(ai->ai_addr, ai->ai_addrlen, ipaddr, sizeof(ipaddr), NULL, 0,
            NI_NUMERICHOST);
        if (ipaddr_list[0] == '\0') sprintf(ipaddr_list, "%s", ipaddr);
        else sprintf(ipaddr_list, "%s;%s", ipaddr_list, ipaddr);
    }
    freeaddrinfo(result);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}

duk_ret_t dnsResolve(duk_context* ctx) {
    char ipaddr[INET6_ADDRSTRLEN];
    void* p;

    const char* host = duk_get_string(ctx, 0);
    if (resolve_host(host, ipaddr, 1, AF_INET)) {
        return 0;
    }
    else {
        p = duk_push_buffer(ctx, strlen(ipaddr) + 1, 0);
        strcpy((char*)p, ipaddr);
    }

    return 1;   /*  1 = return value at top
                 *  0 = return 'undefined'
                 * <0 = throw error (use DUK_RET_xxx constants)
                 */
}

duk_ret_t myIpAddress(duk_context* ctx) {
    char ipaddr[INET6_ADDRSTRLEN];
    void* p;

    char name[256];
    memset(name, '\0', 256);
    gethostname(name, sizeof(name));
    if (resolve_host(name, ipaddr, 1, AF_INET)) {
        p = duk_push_buffer(ctx, strlen("127.0.0.1") + 1, 0);
        strcpy((char*)p, "127.0.0.1");
    }
    else {
        p = duk_push_buffer(ctx, strlen(ipaddr) + 1, 0);
        strcpy((char*)p, ipaddr);
    }

    return 1;   /*  1 = return value at top
                 *  0 = return 'undefined'
                 * <0 = throw error (use DUK_RET_xxx constants)
                 */
}

class duktape_pacrunner : public pacrunner {
public:
	~duktape_pacrunner() {
        duk_destroy_heap(ctx);
	}

    duktape_pacrunner(string pac, const url& pacurl) throw (bad_alloc) : pacrunner(pac, pacurl) {

        this->ctx = duk_create_heap_default();
        if (!ctx) {
            printf("Failed to create a Duktape heap.\n");
            throw bad_alloc();
        }

        /*  PAC support some extra functions that do not always exist in JS.
            More info: https://developer.mozilla.org/en-US/docs/Web/HTTP/Proxy_servers_and_tunneling/Proxy_Auto-Configuration_(PAC)_file */

        /* We first declare the functions dnsResolve and myIpAddress implemented in C*/
        duk_idx_t func_idx;
        func_idx = duk_push_c_function(ctx, dnsResolve, 1);
        duk_put_global_string(ctx, "dnsResolve");

        func_idx = duk_push_c_function(ctx, myIpAddress, 0);
        duk_put_global_string(ctx, "myIpAddress");

        /* We now implement the other functions in JS*/
        duk_push_lstring(ctx, pacUtils, strlen(pacUtils));


        // We push the actual PAC file content
        duk_push_lstring(ctx, pac.c_str(), pac.size());

        if (duk_peval(ctx) != 0) {
            printf("Error: %s\n", duk_safe_to_string(ctx, -1));
            throw bad_alloc();
            //goto finished;
        }
        duk_pop(ctx);  /* ignore result */
	}

	string run(const url& url_) throw (bad_alloc) {
        string      retStr = "";

        duk_push_global_object(ctx);
        if (!duk_get_prop_string(ctx, -1 /*index*/, "FindProxyForURL")) {
            printf("Could not find `%s`. Seems that the PAC file is corrupt.\n", "FindProxyForURL");
            return "";
        }
        duk_push_string(ctx, url_.to_string().c_str());
        duk_push_string(ctx, url_.get_host().c_str());
        if (duk_pcall(ctx, 2 /*nargs*/) != 0) {
            printf("Error: %s\n", duk_safe_to_string(ctx, -1));
            return "";
        }
        else {
            duk_size_t sz;
            const char* tmp_proxy = duk_safe_to_lstring(ctx, -1, &sz);
            if (tmp_proxy != NULL) {
                retStr = string(tmp_proxy, sz);      
                printf("%s\n", tmp_proxy);
            }
        }

        duk_pop(ctx);  /* pop result/error */
        return retStr;
	}

private:
    duk_context* ctx = NULL;
};

PX_PACRUNNER_MODULE_EZ(duktape, "", "duktape");
