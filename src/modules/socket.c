/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
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

// Must stay in sync with the IO_* constants in std/socket/result.kl.
#define IO_OK             0
#define IO_TIMEDOUT       1
#define IO_REFUSED        2
#define IO_RESET          3
#define IO_ABORTED        4
#define IO_ADDR_IN_USE    5
#define IO_ADDR_NOT_AVAIL 6
#define IO_PERMISSION     7
#define IO_INTERRUPTED    8
#define IO_WOULDBLOCK     9
#define IO_INVALID_INPUT  10
#define IO_IO             11

static int _errno_to_io_status(int e)
{
    switch (e) {
        case EINTR:
            return IO_INTERRUPTED;
        case EAGAIN:
            return IO_WOULDBLOCK;
        case ETIMEDOUT:
            return IO_TIMEDOUT;
        case ECONNREFUSED:
            return IO_REFUSED;
        case ECONNRESET:
            return IO_RESET;
        case ECONNABORTED:
            return IO_ABORTED;
        case EADDRINUSE:
            return IO_ADDR_IN_USE;
        case EADDRNOTAVAIL:
            return IO_ADDR_NOT_AVAIL;
        case EACCES:
        case EPERM:
            return IO_PERMISSION;
        case EINVAL:
            return IO_INVALID_INPUT;
        default:
            return IO_IO;
    }
}

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

// func _last_io_status() int {}
static TValue _last_io_status(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    return int64_value(_errno_to_io_status(errno));
}

// func _take_io_error(fd int) int? {}
static TValue _take_io_error(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    int fd = (int)kl_arg_int64(0);

    int err = 0;
    socklen_t len = sizeof(err);
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0) {
        return nil_value;
    }
    if (err == 0) {
        return nil_value;
    }
    return int64_value(_errno_to_io_status(err));
}

// func sys_getsockopt_int(fd int, level int, name int) int {}
static TValue _sys_getsockopt_int(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 3);

    int fd = (int)kl_arg_int64(0);
    int level = (int)kl_arg_int64(1);
    int name = (int)kl_arg_int64(2);

    int val = 0;
    socklen_t len = sizeof(val);
    if (getsockopt(fd, level, name, &val, &len) < 0) {
        return int64_value(-1);
    }
    return int64_value(val);
}

static TValue _from_sockaddr(struct sockaddr *sa, socklen_t salen)
{
    (void)salen;

    if (sa->sa_family == AF_INET) {
        struct sockaddr_in *sin = (struct sockaddr_in *)sa;
        TValue octets = obj_value(kl_bytes_from_data((uint8_t *)&sin->sin_addr, 4));
        TValue v4 = obj_value(kl_new_instance(&ipv4_addr_type));
        ((Ipv4AddrObject *)to_obj(&v4))->octets = octets;

        TValue ip = _new_ip_addr(1, v4);
        TValue port = int64_value(ntohs(sin->sin_port));
        TValue addr = obj_value(kl_new_instance(&socket_addr_type));
        SocketAddrObject *a = (SocketAddrObject *)to_obj(&addr);
        a->ip = ip;
        a->port = port;
        return addr;
    }

    struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;
    TValue octets = obj_value(kl_bytes_from_data((uint8_t *)&sin6->sin6_addr, 16));
    TValue v6 = obj_value(kl_new_instance(&ipv6_addr_type));
    ((Ipv6AddrObject *)to_obj(&v6))->octets = octets;

    TValue ip = _new_ip_addr(0, v6);
    TValue port = int64_value(ntohs(sin6->sin6_port));
    TValue addr = obj_value(kl_new_instance(&socket_addr_type));
    SocketAddrObject *a = (SocketAddrObject *)to_obj(&addr);
    a->ip = ip;
    a->port = port;
    return addr;
}

// func _getsockname(fd int) SocketAddr {}
static TValue _getsockname(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    int fd = (int)kl_arg_int64(0);

    struct sockaddr_storage ss;
    memset(&ss, 0, sizeof(ss));
    ((struct sockaddr *)&ss)->sa_family = AF_INET;
    socklen_t sslen = sizeof(ss);
    getsockname(fd, (struct sockaddr *)&ss, &sslen);
    return _from_sockaddr((struct sockaddr *)&ss, sslen);
}

// func _getpeername(fd int) SocketAddr? {}
static TValue _getpeername(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    int fd = (int)kl_arg_int64(0);

    struct sockaddr_storage ss;
    socklen_t sslen = sizeof(ss);
    if (getpeername(fd, (struct sockaddr *)&ss, &sslen) < 0) {
        return nil_value;
    }
    return _from_sockaddr((struct sockaddr *)&ss, sslen);
}

// func sys_setsockopt_timeout(fd int, name int, ms int) int {}
static TValue _sys_setsockopt_timeout(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 3);

    int fd = (int)kl_arg_int64(0);
    int name = (int)kl_arg_int64(1);
    int ms = (int)kl_arg_int64(2);

    struct timeval tv;
    tv.tv_sec = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;

    int r = setsockopt(fd, SOL_SOCKET, name, &tv, sizeof(tv));
    return int64_value(r);
}

// func sys_fcntl(fd int, cmd int, arg int) int {}
static TValue _sys_fcntl(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 3);

    int fd = (int)kl_arg_int64(0);
    int cmd = (int)kl_arg_int64(1);
    int arg = (int)kl_arg_int64(2);

    return int64_value(fcntl(fd, cmd, arg));
}

// func sys_shutdown(fd int, how int) int {}
static TValue _sys_shutdown(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);

    int fd = (int)kl_arg_int64(0);
    int how = (int)kl_arg_int64(1);

    return int64_value(shutdown(fd, how));
}

// func sys_listen(fd int, backlog int) int {}
static TValue _sys_listen(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);

    int fd = (int)kl_arg_int64(0);
    int backlog = (int)kl_arg_int64(1);

    return int64_value(listen(fd, backlog));
}

// func sys_accept(fd int) (int, SocketAddr?) {}
static TValue _sys_accept(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    int fd = (int)kl_arg_int64(0);

    struct sockaddr_storage ss;
    socklen_t sslen = sizeof(ss);

    int newfd = accept(fd, (struct sockaddr *)&ss, &sslen);

    TValue peer;
    if (newfd < 0) {
        peer = nil_value;
    } else {
        peer = _from_sockaddr((struct sockaddr *)&ss, sslen);
    }

    TValue values[] = {
        int64_value(newfd),
        peer,
    };
    Object *tuple = kl_new_tuple(values, 2);
    return obj_value(tuple);
}

// func sys_sendto(fd int, bs bytes, addr SocketAddr, flags int) int {}
static TValue _sys_sendto(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 4);

    int fd = (int)kl_arg_int64(0);
    BytesObject *bs = kl_arg_obj_as(1, bytes_type);
    SocketAddrObject *addr = kl_arg_obj_as(2, socket_addr_type);
    int flags = (int)kl_arg_int64(3);

    struct sockaddr_storage ss;
    socklen_t sslen = _to_sockaddr(addr, &ss);

    // BYTES_SIZE is the view's length, already accounting for the
    // slice's offset; only the pointer needs the offset added.
    ssize_t n = sendto(fd, BYTES_DATA(bs) + BYTES_OFFSET(bs), BYTES_SIZE(bs), flags,
                       (struct sockaddr *)&ss, sslen);
    return int64_value((int)n);
}

// func sys_recvfrom(fd int, bs bytes, flags int) (int, SocketAddr?) {}
static TValue _sys_recvfrom(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 3);

    int fd = (int)kl_arg_int64(0);
    BytesObject *bs = kl_arg_obj_as(1, bytes_type);
    int flags = (int)kl_arg_int64(2);

    struct sockaddr_storage ss;
    socklen_t sslen = sizeof(ss);

    // Receive straight into the caller's fixed-length array. bytes is
    // not growable, so the caller is responsible for sizing it; a
    // buffer smaller than the datagram truncates the receive.
    ssize_t n = recvfrom(fd, BYTES_DATA(bs) + BYTES_OFFSET(bs), BYTES_SIZE(bs), flags,
                         (struct sockaddr *)&ss, &sslen);

    TValue peer;
    if (n < 0) {
        peer = nil_value;
    } else {
        peer = _from_sockaddr((struct sockaddr *)&ss, sslen);
    }

    TValue values[] = { int64_value((int)n), peer };
    return obj_value(kl_new_tuple(values, 2));
}

// Convert an IpAddr into a sockaddr_storage with port 0.
static socklen_t _ip_to_sockaddr(IpAddrObject *ip, struct sockaddr_storage *ss)
{
    if (to_bool(&ip->is_v4)) {
        Ipv4AddrObject *v4 = (Ipv4AddrObject *)to_obj(&ip->v4);
        BytesObject *octets = (BytesObject *)to_obj(&v4->octets);

        struct sockaddr_in *sin = (struct sockaddr_in *)ss;
        memset(sin, 0, sizeof(*sin));
        sin->sin_family = AF_INET;
        sin->sin_port = 0;
        memcpy(&sin->sin_addr, BYTES_DATA(octets), 4);
        return sizeof(*sin);
    }

    Ipv6AddrObject *v6 = (Ipv6AddrObject *)to_obj(&ip->v6);
    BytesObject *octets = (BytesObject *)to_obj(&v6->octets);

    struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)ss;
    memset(sin6, 0, sizeof(*sin6));
    sin6->sin6_family = AF_INET6;
    sin6->sin6_port = 0;
    memcpy(&sin6->sin6_addr, BYTES_DATA(octets), 16);
    return sizeof(*sin6);
}

static TValue _from_sockaddr_to_ip(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        struct sockaddr_in *sin = (struct sockaddr_in *)sa;
        return _new_ip_addr(1, _new_ipv4_addr(&sin->sin_addr));
    }
    struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;
    return _new_ip_addr(0, _new_ipv6_addr(&sin6->sin6_addr));
}

// func _send_to_ip_from(fd int, bs bytes, ip IpAddr) int {}
static TValue _send_to_ip_from(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 3);

    int fd = (int)kl_arg_int64(0);
    BytesObject *bs = kl_arg_obj_as(1, bytes_type);
    IpAddrObject *ip = kl_arg_obj_as(2, ip_addr_type);

    struct sockaddr_storage ss;
    socklen_t sslen = _ip_to_sockaddr(ip, &ss);

    ssize_t n = sendto(fd, BYTES_DATA(bs) + BYTES_OFFSET(bs), BYTES_SIZE(bs), 0,
                       (struct sockaddr *)&ss, sslen);
    return int64_value((int)n);
}

// func _recv_payload_into(fd int, bs bytes, strip int) (int, IpAddr?) {}
static TValue _recv_payload_into(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 3);

    int fd = (int)kl_arg_int64(0);
    BytesObject *bs = kl_arg_obj_as(1, bytes_type);
    int strip = (int)kl_arg_int64(2);

    struct sockaddr_storage ss;
    socklen_t sslen = sizeof(ss);

    ssize_t n = recvfrom(fd, BYTES_DATA(bs) + BYTES_OFFSET(bs), BYTES_SIZE(bs), 0,
                         (struct sockaddr *)&ss, &sslen);

    TValue from;
    int result_n;

    if (n < 0) {
        from = nil_value;
        result_n = (int)n;
    } else {
        if (strip && n > 0) {
            uint8_t *data = BYTES_DATA(bs) + BYTES_OFFSET(bs);
            uint8_t version = data[0] >> 4;
            size_t hlen = 0;

            if (version == 4) {
                size_t h4 = (size_t)(data[0] & 0x0F) * 4;
                if (h4 >= 20 && (ssize_t)h4 <= n) {
                    hlen = h4;
                }
            } else if (version == 6) {
                if (n >= 40) {
                    hlen = 40;
                }
            }

            if (hlen > 0) {
                // memmove, not memcpy: source and destination are
                // the same allocation.
                memmove(data, data + hlen, (size_t)n - hlen);
                n -= (ssize_t)hlen;
            }
        }
        from = _from_sockaddr_to_ip((struct sockaddr *)&ss);
        result_n = (int)n;
    }

    TValue values[] = { int64_value(result_n), from };
    return obj_value(kl_new_tuple(values, 2));
}

static MethodDef socket_functions[] = {
    { "sys_socket", _sys_socket },
    { "sys_connect", _sys_connect },
    { "sys_bind", _sys_bind },
    { "sys_close", _sys_close },
    { "sys_setsockopt_int", _sys_setsockopt_int },
    { "sys_setsockopt_timeout", _sys_setsockopt_timeout },
    { "sys_getsockopt_int", _sys_getsockopt_int },
    { "sys_send", _sys_send },
    { "sys_recv", _sys_recv },
    { "sys_fcntl", _sys_fcntl },
    { "sys_shutdown", _sys_shutdown },
    { "_getsockname", _getsockname },
    { "_getpeername", _getpeername },
    { "_take_io_error", _take_io_error },
    { "_last_io_status", _last_io_status },
    { "_resolve", resolve_dns },
    { "sys_listen", _sys_listen },
    { "sys_accept", _sys_accept },
    { "sys_sendto", _sys_sendto },
    { "sys_recvfrom", _sys_recvfrom },
    { "_send_to_ip_from", _send_to_ip_from },
    { "_recv_payload_into", _recv_payload_into },
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
