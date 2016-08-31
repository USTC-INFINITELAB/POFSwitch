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

#ifndef _POF_BYTETRANSFER_H_
#define _POF_BYTETRANSFER_H_

/* Define some byte order transfer operators. */
#if( POF_BYTE_ORDER == POF_BIG_ENDIAN )

#define POF_NTOH64(x)                  (x)
#define POF_HTON64(x)                  (x)
#define POF_NTOHL(x)                   (x)
#define POF_HTONL(x)                   (x)
#define POF_NTOHS(x)                   (x)
#define POF_HTONS(x)                   (x)

#else /* POF_BYTE_ORDER == POF_LITTLE_ENDIAN */

#define POF_NTOH64(x)                  ((((x) & 0x00000000000000ffLL) << 56) | \
                                        (((x) & 0x000000000000ff00LL) << 40) | \
                                        (((x) & 0x0000000000ff0000LL) << 24) | \
                                        (((x) & 0x00000000ff000000LL) <<  8) | \
                                        (((x) & 0x000000ff00000000LL) >>  8) | \
                                        (((x) & 0x0000ff0000000000LL) >> 24) | \
                                        (((x) & 0x00ff000000000000LL) >> 40) | \
                                        (((x) & 0xff00000000000000LL) >> 56))

#define POF_HTON64(x)                  ((((x) & 0x00000000000000ffLL) << 56) | \
                                        (((x) & 0x000000000000ff00LL) << 40) | \
                                        (((x) & 0x0000000000ff0000LL) << 24) | \
                                        (((x) & 0x00000000ff000000LL) <<  8) | \
                                        (((x) & 0x000000ff00000000LL) >>  8) | \
                                        (((x) & 0x0000ff0000000000LL) >> 24) | \
                                        (((x) & 0x00ff000000000000LL) >> 40) | \
                                        (((x) & 0xff00000000000000LL) >> 56))

#define POF_NTOHL(x)                   ((((x) & 0x000000ff) << 24) | \
                                        (((x) & 0x0000ff00) <<  8) | \
                                        (((x) & 0x00ff0000) >>  8) | \
                                        (((x) & 0xff000000) >> 24))

#define POF_HTONL(x)                   ((((x) & 0x000000ff) << 24) | \
                                        (((x) & 0x0000ff00) <<  8) | \
                                        (((x) & 0x00ff0000) >>  8) | \
                                        (((x) & 0xff000000) >> 24))

#define POF_NTOHS(x)                   ((((x) & 0x00ff) << 8) | \
                                        (((x) & 0xff00) >> 8))

#define POF_HTONS(x)                   ((((x) & 0x00ff) << 8) | \
                                        (((x) & 0xff00) >> 8))
#endif /* POF_BYTE_ORDER */

/* Define some byte order transfer equation. */
#define POF_NTOH64_FUNC(x) ((x) = POF_NTOH64(x))
#define POF_HTON64_FUNC(x) ((x) = POF_HTON64(x))
#define POF_NTOHL_FUNC(x) ((x) = POF_NTOHL(x))
#define POF_HTONL_FUNC(x) ((x) = POF_HTONL(x))
#define POF_NTOHS_FUNC(x) ((x) = POF_NTOHS(x))
#define POF_HTONS_FUNC(x) ((x) = POF_HTONS(x))

/* Define some byte order transfer function. */
extern uint32_t pof_NtoH_transfer_header(void * ptr);
extern uint32_t pof_HtoN_transfer_header(void * ptr);
extern uint32_t pof_NtoH_transfer_port(void * ptr);
extern uint32_t pof_NtoH_transfer_flow_table(void *ptr);
extern uint32_t pof_NtoH_transfer_flow_entry(void *ptr);
extern uint32_t pof_HtoN_transfer_flow_entry(void *ptr);
extern uint32_t pof_NtoH_transfer_meter(void *ptr);
extern uint32_t pof_NtoH_transfer_group(void *ptr);
extern uint32_t pof_NtoH_transfer_counter(void *ptr);
extern uint32_t pof_HtoN_transfer_switch_features(void *ptr);
extern uint32_t pof_HtoN_transfer_flow_table_resource(void *ptr);
extern uint32_t pof_HtoN_transfer_port_status(void *ptr);
extern uint32_t pof_HtoN_transfer_switch_config(void * ptr);
extern uint32_t pof_HtoN_transfer_queryall_request(void * ptr);
extern uint32_t pof_NtoH_transfer_packet_in(void *ptr);
extern uint32_t pof_NtoH_transfer_error(void *ptr);

#endif // _POF_BYTETRANSFER_H_
