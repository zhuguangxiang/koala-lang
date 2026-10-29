/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <sys/socket.h>
#include "bytesobj.h"
#include "listobj.h"
#include "tupleobj.h"

#ifdef __cplusplus
extern "C" {
#endif

// Must stay in sync with the DNS_* constants in std/socket/result.kl.
#define DNS_OK         0
#define DNS_NOT_FOUND  1
#define DNS_NO_RECORDS 2
#define DNS_TIMEOUT    3
#define DNS_IO         4

typedef struct _Ipv4AddrObject {
    OBJECT_HEAD
    TValue octets;
} Ipv4AddrObject;

TypeObject ipv4_addr_type = {
    ._type = &type_type,
    ._nfields = 1,
    .name = "Ipv4Addr",
    .flags = TP_FLAGS_CLASS,
    .objsize = sizeof(Ipv4AddrObject),
};

typedef struct _Ipv6AddrObject {
    OBJECT_HEAD
    TValue octets;
} Ipv6AddrObject;

TypeObject ipv6_addr_type = {
    ._type = &type_type,
    ._nfields = 1,
    .name = "Ipv6Addr",
    .flags = TP_FLAGS_CLASS,
    .objsize = sizeof(Ipv6AddrObject),
};

// IpAddr carries three fields: the v4/v6 discriminator and one of the
// two address objects. The field that does not match `is_v4` is nil,
// which is the invariant `from_v4` / `from_v6` establish.
typedef struct _IpAddrObject {
    OBJECT_HEAD
    TValue is_v4;
    TValue v4;
    TValue v6;
} IpAddrObject;

TypeObject ip_addr_type = {
    ._type = &type_type,
    ._nfields = 3,
    .name = "IpAddr",
    .flags = TP_FLAGS_CLASS,
    .objsize = sizeof(IpAddrObject),
};

// Build an Ipv4Addr from a 4-byte network-order address.
static TValue _new_ipv4_addr(struct in_addr *a)
{
    Object *obj = kl_new_instance(&ipv4_addr_type);
    Ipv4AddrObject *ipv4 = (Ipv4AddrObject *)obj;
    Object *octets = kl_bytes_from_data((uint8_t *)a, 4);
    ipv4->octets = obj_value(octets);
    return obj_value(obj);
}

// Build an Ipv6Addr from a 16-byte network-order address.
static TValue _new_ipv6_addr(struct in6_addr *a)
{
    Object *obj = kl_new_instance(&ipv6_addr_type);
    Ipv6AddrObject *ipv6 = (Ipv6AddrObject *)obj;
    Object *octets = kl_bytes_from_data((uint8_t *)a, 16);
    ipv6->octets = obj_value(octets);
    return obj_value(obj);
}

// Wrap an address object into an IpAddr. `is_v4` selects which field
// is populated; the other is set to nil, matching what from_v4 /
// from_v6 do at the Koala level.
static TValue _new_ip_addr(int is_v4, TValue addr)
{
    Object *obj = kl_new_instance(&ip_addr_type);
    IpAddrObject *ip = (IpAddrObject *)obj;

    ip->is_v4 = bool_value(is_v4 != 0);
    if (is_v4) {
        ip->v4 = addr;
        ip->v6 = nil_value;
    } else {
        ip->v4 = nil_value;
        ip->v6 = addr;
    }
    return obj_value(obj);
}

// Package (status, addrs, message) into the tuple that @native
// multi-value returns use. A null `list` is replaced with an empty
// list, so failure cases never return a null second element.
static TValue _resolve_tuple(int status, Object *list, const char *msg)
{
    Object *lst = list ? list : kl_new_list();
    TValue values[] = {
        int64_value(status),
        obj_value(lst),
        obj_value(msg),
    };
    Object *tuple = kl_new_tuple(values, 3);
    return obj_value(tuple);
}

/*
func _resolve(host str) (int, list[IpAddr], str) {}
*/
static TValue resolve_dns(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    char *host = kl_arg_str(0);

    // Numeric IPv4.
    struct in_addr a4;
    if (inet_pton(AF_INET, host, &a4) == 1) {
        TValue ip = _new_ip_addr(1, _new_ipv4_addr(&a4));
        Object *list = kl_new_list();
        kl_list_append(list, ip);
        return _resolve_tuple(DNS_OK, list, "");
    }

    // Numeric IPv6.
    struct in6_addr a6;
    if (inet_pton(AF_INET6, host, &a6) == 1) {
        TValue ip = _new_ip_addr(0, _new_ipv6_addr(&a6));
        Object *list = kl_new_list();
        kl_list_append(list, ip);
        return _resolve_tuple(DNS_OK, list, "");
    }

    // Name resolution. AF_UNSPEC returns both A and AAAA records; the
    // order is the one the system picked per RFC 3484, so `dial` can
    // take the first entry as the preferred address.
    struct addrinfo hints;
    struct addrinfo *res = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int rc = getaddrinfo(host, NULL, &hints, &res);
    if (rc != 0) {
        int status;
        switch (rc) {
            case EAI_NONAME:
                status = DNS_NOT_FOUND;
                break;
            case EAI_AGAIN:
                status = DNS_TIMEOUT;
                break;
            default:
                status = DNS_IO;
                break;
        }
        return _resolve_tuple(status, NULL, gai_strerror(rc));
    }

    Object *list = kl_new_list();
    int count = 0;

    for (struct addrinfo *ai = res; ai != NULL; ai = ai->ai_next) {
        TValue ip = nil_value;

        if (ai->ai_family == AF_INET) {
            struct sockaddr_in *sin = (struct sockaddr_in *)ai->ai_addr;
            ip = _new_ip_addr(1, _new_ipv4_addr(&sin->sin_addr));
        } else if (ai->ai_family == AF_INET6) {
            struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)ai->ai_addr;
            ip = _new_ip_addr(0, _new_ipv6_addr(&sin6->sin6_addr));
        } else {
            continue;
        }

        kl_list_append(list, ip);
        count++;
    }
    freeaddrinfo(res);

    if (count == 0) {
        return _resolve_tuple(DNS_NO_RECORDS, NULL, "no A/AAAA records");
    }
    return _resolve_tuple(DNS_OK, list, "");
}

static MethodDef socket_functions[] = {
    { "_resolve", resolve_dns },
    { NULL },
};

void socket_native_lib_init(NativeLib *lib)
{
    MethodDef *methdef = socket_functions;
    while (methdef->name) {
        kl_reg_func(lib, methdef->name, methdef->cfunc);
        ++methdef;
    }

    kl_reg_type(lib, &ipv4_addr_type);
    kl_reg_type(lib, &ipv6_addr_type);
    kl_reg_type(lib, &ip_addr_type);
}

#ifdef __cplusplus
}
#endif
