/**
 * Copyright (c) 2012, 2013, Huawei Technologies Co., Ltd.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met: 
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _POF_PROTOCOL_HEADER_H_
#define _POF_PROTOCOL_HEADER_H_

#include "pof_type.h"
#include "pof_global.h"

#define POF_ETH_TYPE_IPV4   (0x0800)

#define POF_IPV4_VERSION            (4)
#define POF_IPV4_HEADER_LEN         (5)
#define POF_IPV4_ID_DEFAULT         (0)
#define POF_IPV4_FRAG_OFF_DEFAULT   (0)
#define POF_IPV4_TTL_DEFAULT        (64)
#define POF_IPV4_PROTOCOL_UDP       (17)
#define POF_IPV4_ALEN               (4)

#define POF_UDP_PORT_VXLAN  (0x4789)
#define POF_VXLAN_FLAG      (0x08)
#define POF_VXLAN_VNI_LEN   (3)

struct eth_header {
    uint8_t dmac[POF_ETH_ALEN];
    uint8_t smac[POF_ETH_ALEN];
    uint16_t ethType;
};

struct ipv4_header {
#if ( POF_BYTE_ORDER == POF_LITTLE_ENDIAN )
    uint8_t ihl     :4;
    uint8_t version :4;
#else // POF_BYTE_ORDER
    uint8_t version :4;
    uint8_t ihl     :4;
#endif // POF_BYTE_ORDER
    uint8_t tos;
    uint16_t totalLen;
    uint16_t id;
    uint16_t frag_off;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checkSum;
    uint32_t sip;
    uint32_t dip;
};

struct udp_header {
    uint16_t sport;
    uint16_t dport;
    uint16_t len;
    uint16_t check;
};

struct vxlan_header {
    uint8_t flag;
    uint8_t rsv1[3];
    uint8_t vni[3];
    uint8_t rsv2;
};

#endif // _POF_PROTOCOL_HEADER_H_
