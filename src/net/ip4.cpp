// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#define DEBUG // Allow debugging
#define DEBUG2 // Allow debug lvl 2
#include <os>
#include <net/ip4.hpp>
#include <net/ip4/packet_ip4.hpp>
#include <net/packet.hpp>

namespace net {

const IP4::addr IP4::INADDR_ANY   {{0,0,0,0}};
const IP4::addr IP4::INADDR_BCAST {{0xff,0xff,0xff,0xff}};

IP4::IP4(Inet<LinkLayer, IP4>& inet) noexcept:
  stack_{inet}
{
  // Default gateway is addr 1 in the subnet.
  // const uint32_t DEFAULT_GATEWAY = htonl(1);
  // gateway_.whole = (local_ip_.whole & netmask_.whole) | DEFAULT_GATEWAY;
}

void IP4::bottom(Packet_ptr pckt) {
  debug2("<IP4 handler> got the data.\n");
    
  auto data = pckt->buffer();
  ip_header* hdr = &reinterpret_cast<full_header*>(data)->ip_hdr;
  
  debug2("\t Source IP: %s Dest.IP: %s\n",
    hdr->saddr.str().c_str(), hdr->daddr.str().c_str());
  
  switch(hdr->protocol){
  case IP4_ICMP:
    debug2("\t Type: ICMP\n");
    icmp_handler_(pckt);
    break;
  case IP4_UDP:
    debug2("\t Type: UDP\n");
    udp_handler_(pckt);
    break;
  case IP4_TCP:
    tcp_handler_(pckt);
    debug2("\t Type: TCP\n");
    break;
  default:
    debug("\t Type: UNKNOWN %i\n", hdr->protocol);
    break;
  }
}

uint16_t IP4::checksum(ip_header* hdr) {
  return net::checksum(reinterpret_cast<uint16_t*>(hdr), sizeof(ip_header));
}

void IP4::transmit(Packet_ptr pckt) {
  assert(pckt->size() > sizeof(IP4::full_header));    
  
  full_header* full_hdr = reinterpret_cast<full_header*>(pckt->buffer());
  ip_header* hdr = &full_hdr->ip_hdr;
  
  auto ip4_pckt = std::static_pointer_cast<PacketIP4>(pckt);
  ip4_pckt->make_flight_ready();
  
  // Create local and target subnets
  addr target, local;
  target.whole = hdr->daddr.whole       & stack_.netmask().whole;
  local.whole  = stack_.ip_addr().whole & stack_.netmask().whole;
  
  // Compare subnets to know where to send packet
  pckt->next_hop(target == local ? hdr->daddr : stack_.router());
  
  debug("<IP4 TOP> Next hop for %s, (netmask %s, local IP: %s, gateway: %s) == %s\n",
      hdr->daddr.str().c_str(),
      stack_.netmask().str().c_str(),
      stack_.ip_addr().str().c_str(),
      stack_.router().str().c_str(),
      target == local ? "DIRECT" : "GATEWAY");
  
  debug("<IP4 transmit> my ip: %s, Next hop: %s, Packet size: %i IP4-size: %i\n",
      stack_.ip_addr().str().c_str(),
      pckt->next_hop().str().c_str(),
      pckt->size(),
      ip4_pckt->ip4_segment_size()
	);
  
  linklayer_out_(pckt);
}

// Empty handler for delegates initialization
void ignore_ip4_up(Packet_ptr UNUSED(pckt)) {
  debug("<IP4> Empty handler. Ignoring.\n");
}

void ignore_ip4_down(Packet_ptr UNUSED(pckt)) {
  debug("<IP4->Link layer> No handler - DROP!\n");
}

} //< namespace net
