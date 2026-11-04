/*
 * Created by v1tr10l7 on 10.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Posix/bits/socket.h>

/* Routing/device hook				*/
constexpr usize NETLINK_ROUTE          = 0;
/* Unused number				*/
constexpr usize NETLINK_UNUSED         = 1;
/* Reserved for user mode socket protocols 	*/
constexpr usize NETLINK_USERSOCK       = 2;
/* Unused number, formerly ip_queue		*/
constexpr usize NETLINK_FIREWALL       = 3;
/* socket monitoring    */
constexpr usize NETLINK_SOCK_DIAG      = 4;
/* netfilter/iptables ULOG */
constexpr usize NETLINK_NFLOG          = 5;
/* ipsec */
constexpr usize NETLINK_XFRM           = 6;
/* SELinux event notifications */
constexpr usize NETLINK_SELINUX        = 7;
/* Open-iSCSI */
constexpr usize NETLINK_ISCSI          = 8;
/* auditing */
constexpr usize NETLINK_AUDIT          = 9;
constexpr usize NETLINK_FIB_LOOKUP     = 10;
constexpr usize NETLINK_CONNECTOR      = 11;
/* netfilter subsystem */
constexpr usize NETLINK_NETFILTER      = 12;
constexpr usize NETLINK_IP6_FW         = 13;
/* DECnet routing messages (obsolete) */
constexpr usize NETLINK_DNRTMSG        = 14;
/* Kernel messages to userspace */
constexpr usize NETLINK_KOBJECT_UEVENT = 15;
constexpr usize NETLINK_GENERIC        = 16;
/* leave room for NETLINK_DM (DM Events) */
/* SCSI Transports */
constexpr usize NETLINK_SCSITRANSPORT  = 18;
constexpr usize NETLINK_ECRYPTFS       = 19;
constexpr usize NETLINK_RDMA           = 20;
/* Crypto layer */
constexpr usize NETLINK_CRYPTO         = 21;
/* SMC monitoring */
constexpr usize NETLINK_SMC            = 22;

constexpr usize NETLINK_INET_DIAG      = NETLINK_SOCK_DIAG;

constexpr usize MAX_LINKS              = 32;

struct sockaddr_nl
{
    sa_family_t    nl_family; /* AF_NETLINK	*/
    unsigned short nl_pad;    /* zero		*/
    u32            nl_pid;    /* port ID	*/
    u32            nl_groups; /* multicast groups mask */
};

/**
 * struct nlmsghdr - fixed format metadata header of Netlink messages
 * @nlmsg_len:   Length of message including header
 * @nlmsg_type:  Message content type
 * @nlmsg_flags: Additional flags
 * @nlmsg_seq:   Sequence number
 * @nlmsg_pid:   Sending process port ID
 */
struct nlmsghdr
{
    u32 nlmsg_len;
    u16 nlmsg_type;
    u16 nlmsg_flags;
    u32 nlmsg_seq;
    u32 nlmsg_pid;
};

/* Flags values */

/* It is request message. 	*/
constexpr usize NLM_F_REQUEST       = 0x01;
/* Multipart message, terminated by NLMSG_DONE */
constexpr usize NLM_F_MULTI         = 0x02;
/* Reply with ack, with zero or error code */
constexpr usize NLM_F_ACK           = 0x04;
/* Receive resulting notifications */
constexpr usize NLM_F_ECHO          = 0x08;
/* Dump was inconsistent due to sequence change \
constexpr usize NLM_F_DUMP_INTR = 0x10;
                                  */
/* Dump was filtered as requested */
constexpr usize NLM_F_DUMP_FILTERED = 0x20;

/* Modifiers to GET request */
/* specify tree	root	*/
constexpr usize NLM_F_ROOT          = 0x100;
/* return all matching	*/
constexpr usize NLM_F_MATCH         = 0x200;
/* atomic GET		*/
constexpr usize NLM_F_ATOMIC        = 0x400;
constexpr usize NLM_F_DUMP          = (NLM_F_ROOT | NLM_F_MATCH);

/* Modifiers to NEW request */
/* Override existing		*/
constexpr usize NLM_F_REPLACE       = 0x100;
/* Do not touch, if it exists	*/
constexpr usize NLM_F_EXCL          = 0x200;
/* Create, if it does not exist	*/
constexpr usize NLM_F_CREATE        = 0x400;
/* Add to end of list		*/
constexpr usize NLM_F_APPEND        = 0x800;

/* Modifiers to DELETE request */
/* Do not delete recursively	*/
constexpr usize NLM_F_NONREC        = 0x100;
/* Delete multiple objects	*/
constexpr usize NLM_F_BULK          = 0x200;

/* Flags for ACK message */
/* request was capped */
constexpr usize NLM_F_CAPPED        = 0x100;
/* extended ACK TVLs were included */
constexpr usize NLM_F_ACK_TLVS      = 0x200;

/*
   4.4BSD ADD		NLM_F_CREATE|NLM_F_EXCL
   4.4BSD CHANGE	NLM_F_REPLACE

   True CHANGE		NLM_F_CREATE|NLM_F_REPLACE
   Append		NLM_F_CREATE
   Check		NLM_F_EXCL
 */

constexpr usize NLMSG_ALIGNTO       = 4u;
constexpr usize NLMSG_ALIGN(usize len)
{
    return (((len) + NLMSG_ALIGNTO - 1) & ~(NLMSG_ALIGNTO - 1));
};
constexpr usize NLMSG_HDRLEN()
{
    return ((int)NLMSG_ALIGN(sizeof(struct nlmsghdr)));
};
constexpr usize NLMSG_LENGTH(usize len) { return ((len) + NLMSG_HDRLEN()); }
constexpr usize NLMSG_SPACE(usize len)
{
    return NLMSG_ALIGN(NLMSG_LENGTH(len));
}
constexpr usize NLMSG_DATA(usize nlh) { return nlh + NLMSG_HDRLEN(); }

/* Nothing.		*/
constexpr usize NLMSG_NOOP     = 0x1;
/* Error		*/
constexpr usize NLMSG_ERROR    = 0x2;
/* End of a dump	*/
constexpr usize NLMSG_DONE     = 0x3;
/* Data lost		*/
constexpr usize NLMSG_OVERRUN  = 0x4;

/* < 0x10: reserved control messages */
constexpr usize NLMSG_MIN_TYPE = 0x10;

struct nlmsgerr
{
    int             error;
    struct nlmsghdr msg;
    /*
     * followed by the message contents unless NETLINK_CAP_ACK was set
     * or the ACK indicates success (error == 0)
     * message length is aligned with NLMSG_ALIGN()
     */
    /*
     * followed by TLVs defined in enum nlmsgerr_attrs
     * if NETLINK_EXT_ACK was set
     */
};

/**
 * enum nlmsgerr_attrs - nlmsgerr attributes
 * @NLMSGERR_ATTR_UNUSED: unused
 * @NLMSGERR_ATTR_MSG: error message string (string)
 * @NLMSGERR_ATTR_OFFS: offset of the invalid attribute in the original
 *	 message, counting from the beginning of the header (u32)
 * @NLMSGERR_ATTR_COOKIE: arbitrary subsystem specific cookie to
 *	be used - in the success case - to identify a created
 *	object or operation or similar (binary)
 * @NLMSGERR_ATTR_POLICY: policy for a rejected attribute
 * @NLMSGERR_ATTR_MISS_TYPE: type of a missing required attribute,
 *	%NLMSGERR_ATTR_MISS_NEST will not be present if the attribute was
 *	missing at the message level
 * @NLMSGERR_ATTR_MISS_NEST: offset of the nest where attribute was missing
 * @__NLMSGERR_ATTR_MAX: number of attributes
 * @NLMSGERR_ATTR_MAX: highest attribute number
 */
enum nlmsgerr_attrs
{
    NLMSGERR_ATTR_UNUSED,
    NLMSGERR_ATTR_MSG,
    NLMSGERR_ATTR_OFFS,
    NLMSGERR_ATTR_COOKIE,
    NLMSGERR_ATTR_POLICY,
    NLMSGERR_ATTR_MISS_TYPE,
    NLMSGERR_ATTR_MISS_NEST,

    __NLMSGERR_ATTR_MAX,
    NLMSGERR_ATTR_MAX = __NLMSGERR_ATTR_MAX - 1
};

constexpr usize NETLINK_ADD_MEMBERSHIP   = 1;
constexpr usize NETLINK_DROP_MEMBERSHIP  = 2;
constexpr usize NETLINK_PKTINFO          = 3;
constexpr usize NETLINK_BROADCAST_ERROR  = 4;
constexpr usize NETLINK_NO_ENOBUFS       = 5;
constexpr usize NETLINK_RX_RING          = 6;
constexpr usize NETLINK_TX_RING          = 7;
constexpr usize NETLINK_LISTEN_ALL_NSID  = 8;
constexpr usize NETLINK_LIST_MEMBERSHIPS = 9;
constexpr usize NETLINK_CAP_ACK          = 10;
constexpr usize NETLINK_EXT_ACK          = 11;
constexpr usize NETLINK_GET_STRICT_CHK   = 12;

struct nl_pktinfo
{
    u32 group;
};

struct nl_mmap_req
{
    unsigned int nm_block_size;
    unsigned int nm_block_nr;
    unsigned int nm_frame_size;
    unsigned int nm_frame_nr;
};

struct nl_mmap_hdr
{
    unsigned int nm_status;
    unsigned int nm_len;
    u32          nm_group;
    /* credentials */
    u32          nm_pid;
    u32          nm_uid;
    u32          nm_gid;
};

enum nl_mmap_status
{
    NL_MMAP_STATUS_UNUSED,
    NL_MMAP_STATUS_RESERVED,
    NL_MMAP_STATUS_VALID,
    NL_MMAP_STATUS_COPY,
    NL_MMAP_STATUS_SKIP,
};

constexpr usize NL_MMAP_MSG_ALIGNMENT = NLMSG_ALIGNTO;
constexpr usize NL_MMAP_MSG_ALIGN(usize sz)
{
    return (sz + NL_MMAP_MSG_ALIGNMENT) & ~NL_MMAP_MSG_ALIGNMENT;
}
constexpr usize NL_MMAP_HDRLEN = NL_MMAP_MSG_ALIGN(sizeof(struct nl_mmap_hdr));

/* Major 36 is reserved for networking*/
constexpr usize NET_MAJOR      = 36;

enum
{
    NETLINK_UNCONNECTED = 0,
    NETLINK_CONNECTED,
};

/*
 *  <------- NLA_HDRLEN ------> <-- NLA_ALIGN(payload)-->
 * +---------------------+- - -+- - - - - - - - - -+- - -+
 * |        Header       | Pad |     Payload       | Pad |
 * |   (struct nlattr)   | ing |                   | ing |
 * +---------------------+- - -+- - - - - - - - - -+- - -+
 *  <-------------- nlattr->nla_len -------------->
 */

struct nlattr
{
    u16 nla_len;
    u16 nla_type;
};

/*
 * nla_type (16 bits)
 * +---+---+-------------------------------+
 * | N | O | Attribute Type                |
 * +---+---+-------------------------------+
 * N := Carries nested attributes
 * O := Payload stored in network byte order
 *
 * Note: The N and O flag are mutually exclusive.
 */
constexpr usize NLA_F_NESTED        = (1 << 15);
constexpr usize NLA_F_NET_BYTEORDER = (1 << 14);
constexpr usize NLA_TYPE_MASK       = ~(NLA_F_NESTED | NLA_F_NET_BYTEORDER);

constexpr usize NLA_ALIGNTO         = 4;
constexpr usize NLA_ALIGN(usize len)
{
    return (((len) + NLA_ALIGNTO - 1) & ~(NLA_ALIGNTO - 1));
}
constexpr usize NLA_HDRLEN = ((int)NLA_ALIGN(sizeof(struct nlattr)));

/* Generic 32 bitflags attribute content sent to the kernel.
 *
 * The value is a bitmap that defines the values being set
 * The selector is a bitmask that defines which value is legit
 *
 * Examples:
 *  value = 0x0, and selector = 0x1
 *  implies we are selecting bit 1 and we want to set its value to 0.
 *
 *  value = 0x2, and selector = 0x2
 *  implies we are selecting bit 2 and we want to set its value to 1.
 *
 */
struct nla_bitfield32
{
    u32 value;
    u32 selector;
};

/*
 * policy descriptions - it's specific to each family how
 * this is used Normally, it should be retrieved via a dump
 * inside another attribute specifying where it applies.
 */

/**
 * enum netlink_attribute_type - type of an attribute
 * @NL_ATTR_TYPE_INVALID: unused
 * @NL_ATTR_TYPE_FLAG: flag attribute (present/not present)
 * @NL_ATTR_TYPE_U8: 8-bit unsigned attribute
 * @NL_ATTR_TYPE_U16: 16-bit unsigned attribute
 * @NL_ATTR_TYPE_U32: 32-bit unsigned attribute
 * @NL_ATTR_TYPE_U64: 64-bit unsigned attribute
 * @NL_ATTR_TYPE_S8: 8-bit signed attribute
 * @NL_ATTR_TYPE_S16: 16-bit signed attribute
 * @NL_ATTR_TYPE_S32: 32-bit signed attribute
 * @NL_ATTR_TYPE_S64: 64-bit signed attribute
 * @NL_ATTR_TYPE_BINARY: binary data, min/max length may be
 *specified
 * @NL_ATTR_TYPE_STRING: string, min/max length may be
 *specified
 * @NL_ATTR_TYPE_NUL_STRING: NUL-terminated string,
 *	min/max length may be specified
 * @NL_ATTR_TYPE_NESTED: nested, i.e. the content of this
 *attribute consists of sub-attributes. The nested policy and
 *maxtype inside may be specified.
 * @NL_ATTR_TYPE_NESTED_ARRAY: nested array, i.e. the content
 *of this attribute contains sub-attributes whose type is
 *irrelevant (just used to separate the array entries) and
 *each such array entry has attributes again, the policy for
 *those inner ones and the corresponding maxtype may be
 *specified.
 * @NL_ATTR_TYPE_BITFIELD32: &struct nla_bitfield32 attribute
 * @NL_ATTR_TYPE_SINT: 32-bit or 64-bit signed attribute,
 *aligned to 4B
 * @NL_ATTR_TYPE_UINT: 32-bit or 64-bit unsigned attribute,
 *aligned to 4B
 */
enum netlink_attribute_type
{
    NL_ATTR_TYPE_INVALID,

    NL_ATTR_TYPE_FLAG,

    NL_ATTR_TYPE_U8,
    NL_ATTR_TYPE_U16,
    NL_ATTR_TYPE_U32,
    NL_ATTR_TYPE_U64,

    NL_ATTR_TYPE_S8,
    NL_ATTR_TYPE_S16,
    NL_ATTR_TYPE_S32,
    NL_ATTR_TYPE_S64,

    NL_ATTR_TYPE_BINARY,
    NL_ATTR_TYPE_STRING,
    NL_ATTR_TYPE_NUL_STRING,

    NL_ATTR_TYPE_NESTED,
    NL_ATTR_TYPE_NESTED_ARRAY,

    NL_ATTR_TYPE_BITFIELD32,

    NL_ATTR_TYPE_SINT,
    NL_ATTR_TYPE_UINT,
};

/**
 * enum netlink_policy_type_attr - policy type attributes
 * @NL_POLICY_TYPE_ATTR_UNSPEC: unused
 * @NL_POLICY_TYPE_ATTR_TYPE: type of the attribute,
 *	&enum netlink_attribute_type (U32)
 * @NL_POLICY_TYPE_ATTR_MIN_VALUE_S: minimum value for signed
 *	integers (S64)
 * @NL_POLICY_TYPE_ATTR_MAX_VALUE_S: maximum value for signed
 *	integers (S64)
 * @NL_POLICY_TYPE_ATTR_MIN_VALUE_U: minimum value for
 *unsigned integers (U64)
 * @NL_POLICY_TYPE_ATTR_MAX_VALUE_U: maximum value for
 *unsigned integers (U64)
 * @NL_POLICY_TYPE_ATTR_MIN_LENGTH: minimum length for binary
 *	attributes, no minimum if not given (U32)
 * @NL_POLICY_TYPE_ATTR_MAX_LENGTH: maximum length for binary
 *	attributes, no maximum if not given (U32)
 * @NL_POLICY_TYPE_ATTR_POLICY_IDX: sub policy for nested and
 *	nested array types (U32)
 * @NL_POLICY_TYPE_ATTR_POLICY_MAXTYPE: maximum sub policy
 *	attribute for nested and nested array types, this can
 *	in theory be < the size of the policy pointed to by
 *	the index, if limited inside the nesting (U32)
 * @NL_POLICY_TYPE_ATTR_BITFIELD32_MASK: valid mask for the
 *	bitfield32 type (U32)
 * @NL_POLICY_TYPE_ATTR_MASK: mask of valid bits for unsigned
 *integers (U64)
 * @NL_POLICY_TYPE_ATTR_PAD: pad attribute for 64-bit
 *alignment
 *
 * @__NL_POLICY_TYPE_ATTR_MAX: number of attributes
 * @NL_POLICY_TYPE_ATTR_MAX: highest attribute number
 */
enum netlink_policy_type_attr
{
    NL_POLICY_TYPE_ATTR_UNSPEC,
    NL_POLICY_TYPE_ATTR_TYPE,
    NL_POLICY_TYPE_ATTR_MIN_VALUE_S,
    NL_POLICY_TYPE_ATTR_MAX_VALUE_S,
    NL_POLICY_TYPE_ATTR_MIN_VALUE_U,
    NL_POLICY_TYPE_ATTR_MAX_VALUE_U,
    NL_POLICY_TYPE_ATTR_MIN_LENGTH,
    NL_POLICY_TYPE_ATTR_MAX_LENGTH,
    NL_POLICY_TYPE_ATTR_POLICY_IDX,
    NL_POLICY_TYPE_ATTR_POLICY_MAXTYPE,
    NL_POLICY_TYPE_ATTR_BITFIELD32_MASK,
    NL_POLICY_TYPE_ATTR_PAD,
    NL_POLICY_TYPE_ATTR_MASK,

    /* keep last */
    __NL_POLICY_TYPE_ATTR_MAX,
    NL_POLICY_TYPE_ATTR_MAX = __NL_POLICY_TYPE_ATTR_MAX - 1
};
