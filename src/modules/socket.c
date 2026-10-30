/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
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

typedef struct _SocketAddrObject {
    OBJECT_HEAD
    TValue ip;   // IpAddr
    TValue port; // int
} SocketAddrObject;

TypeObject socket_addr_type = {
    ._type = &type_type,
    ._nfields = 2,
    .name = "SocketAddr",
    .flags = TP_FLAGS_CLASS,
    .objsize = sizeof(SocketAddrObject),
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

// func sys_socket(domain int, type_ int, proto int) int {}
static TValue _sys_socket(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 3);

    int domain = (int)kl_arg_int64(0);
    int type_ = (int)kl_arg_int64(1);
    int protocol = (int)kl_arg_int64(2);

    int fd = socket(domain, type_, protocol);
    printf("_sys_socket: %d, protocal: %d, type: %d\n", fd, protocol, type_);
    return int64_value(fd);
}

// Convert a Koala SocketAddr into a sockaddr_storage. Returns the
// sockaddr length, or 0 on failure.
//
// The address bytes are already in network order (Ipv4Addr / Ipv6Addr
// store them that way), so they are copied verbatim into sin_addr /
// sin6_addr. Only the port needs htons().
static socklen_t _to_sockaddr(SocketAddrObject *addr, struct sockaddr_storage *ss)
{
    IpAddrObject *ip = (IpAddrObject *)to_obj(&addr->ip);
    int port = (int)to_int64(&addr->port);

    if (to_bool(&ip->is_v4)) {
        Ipv4AddrObject *v4 = (Ipv4AddrObject *)to_obj(&ip->v4);
        BytesObject *octets = (BytesObject *)to_obj(&v4->octets);

        struct sockaddr_in *sin = (struct sockaddr_in *)ss;
        memset(sin, 0, sizeof(*sin));
        sin->sin_family = AF_INET;
        sin->sin_port = htons((uint16_t)port);
        memcpy(&sin->sin_addr, octets->data, 4);
        return sizeof(*sin);
    }

    Ipv6AddrObject *v6 = (Ipv6AddrObject *)to_obj(&ip->v6);
    BytesObject *octets = (BytesObject *)to_obj(&v6->octets);

    struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)ss;
    memset(sin6, 0, sizeof(*sin6));
    sin6->sin6_family = AF_INET6;
    sin6->sin6_port = htons((uint16_t)port);
    memcpy(&sin6->sin6_addr, octets->data, 16);
    return sizeof(*sin6);
}

// func sys_connect(fd int, addr SocketAddr) int {}
static TValue _sys_connect(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);

    int fd = (int)kl_arg_int64(0);
    SocketAddrObject *addr = kl_arg_obj_as(1, socket_addr_type);

    struct sockaddr_storage ss;
    socklen_t sslen = _to_sockaddr(addr, &ss);

    int r = connect(fd, (struct sockaddr *)&ss, sslen);
    return int64_value(r);
}

static TValue _sys_bind(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);

    int fd = (int)kl_arg_int64(0);
    SocketAddrObject *addr = kl_arg_obj_as(1, socket_addr_type);

    struct sockaddr_storage ss;
    socklen_t sslen = _to_sockaddr(addr, &ss);

    int r = bind(fd, (struct sockaddr *)&ss, sslen);
    return int64_value(r);
}

// func sys_close(fd int) int {}
static TValue _sys_close(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);

    int fd = (int)kl_arg_int64(0);
    int r = close(fd);
    return int64_value(r);
}

// func sys_setsockopt_int(fd int, level int, name int, val int) int {}
static TValue _sys_setsockopt_int(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 4);

    int fd = (int)kl_arg_int64(0);
    int level = (int)kl_arg_int64(1);
    int name = (int)kl_arg_int64(2);
    int val = (int)kl_arg_int64(3);

    int r = setsockopt(fd, level, name, &val, sizeof(val));
    return int64_value(r);
}

// func sys_send(fd int, bs bytes, flags int) int {}
static TValue _sys_send(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 3);

    int fd = (int)kl_arg_int64(0);
    printf("_sys_send: %d\n", fd);
    BytesObject *bs = kl_arg_obj_as(1, bytes_type);
    int flags = (int)kl_arg_int64(2);

    ssize_t n = send(fd, bs->data + bs->offset, bs->size, flags);
    return int64_value((int)n);
}

// func sys_recv(fd int, bs bytes, flags int) int {}
static TValue _sys_recv(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 3);

    int fd = (int)kl_arg_int64(0);
    BytesObject *bs = kl_arg_obj_as(1, bytes_type);
    int flags = (int)kl_arg_int64(2);

    ssize_t n = recv(fd, bs->data + bs->offset, bs->size, flags);
    return int64_value((int)n);
}

static MethodDef socket_functions[] = {
    { "sys_socket", _sys_socket },
    { "sys_connect", _sys_connect },
    { "sys_bind", _sys_bind },
    { "sys_close", _sys_close },
    { "sys_setsockopt_int", _sys_setsockopt_int },
    { "sys_send", _sys_send },
    { "sys_recv", _sys_recv },
    { "_resolve", resolve_dns },
    { NULL },
};

void socket_native_lib_init(NativeLib *lib)
{
    signal(SIGPIPE, SIG_IGN);

    MethodDef *methdef = socket_functions;
    while (methdef->name) {
        kl_reg_func(lib, methdef->name, methdef->cfunc);
        ++methdef;
    }

    kl_reg_type(lib, &ipv4_addr_type);
    kl_reg_type(lib, &ipv6_addr_type);
    kl_reg_type(lib, &ip_addr_type);
    kl_reg_type(lib, &socket_addr_type);
}

#ifdef __cplusplus
}
#endif
