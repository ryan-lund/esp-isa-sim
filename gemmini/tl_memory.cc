#include "gemmini_params.h"
#include "tl_memory.h"
#include <bitset>
#include <cmath>
#include <iostream>
#include <string>
#include <unistd.h>
#include <fstream>
#include <fcntl.h>
#include <google/protobuf/util/delimited_message_util.h>

 void tl_memory_t::run() {
  verif::TLA tla;

  while(true) {
    tla_ifs->open((std::string(cosim_path) + "/TLAPipe").c_str(), std::fstream::in | std::fstream::binary);

    while (!tla.ParseFromIstream(tla_ifs)) {}

    //printf("[TL MEMORY] Received new tla: %s \n", tla.DebugString().c_str());

    tla_ifs->close();
    processTLA(&tla);
  }
}

// Debug run command for manually testing TLA bundles
void tl_memory_t::debug_run(verif::TLA *tla) {
  processTLA(tla);
}

void tl_memory_t::processTLA(verif::TLA *tla) {
//  printf("[TL MEMORY] Processing TL-A \n");
  if (!tla) {
    return;
  }

//  printf("[TL MEMORY] TL-A opcode %u\n", tla->opcode());
//  printf("[TL MEMORY] TL-A size %u\n", tla->size());
//  printf("[TL MEMORY] TL-A source %u\n", tla->source());
//  printf("[TL MEMORY] TL-A address %u\n", tla->address());

  switch(tla->opcode()) {
    case 0:
      putFullData(tla);
      break;
    case 1:
      putPartialData(tla);
      break;
    case 2:
      //TODO: ArithmeticData
      printf("[TL MEMORY] No registered operation for opcode %u\n", tla->opcode());
    case 3:
      //TODO: LogicalData
      printf("[TL MEMORY] No registered operation for opcode %u\n", tla->opcode());
      break;
    case 5:
      //TODO: Intent
      printf("[TL MEMORY] No registered operation for opcode %u\n", tla->opcode());
      break;
    case 4:
      get(tla);
      break;
    default:
      printf("[TL MEMORY] No registered operation for opcode %u\n", tla->opcode());
      break;
  }

  tla->Clear();
  return;
}

void tl_memory_t::putFullData(verif::TLA *tla, size_t prev_visited_bytes) {
  if (tla->param() != 0 || tla->corrupt() != 0 || (tla->address() & ((1 << tla->size()) - 1)) != 0) {
    printf("[TL_MEMORY] Doing nothing for malformed TL-A: \n");
    printf("%s \n", tla->DebugString().c_str());
    return;
  }

//  printf("Processing putFullData\n");

  std::bitset<16> mask(tla->mask());

  // Start  at first non-zero entry of byte mask
  size_t nonzero_byte = 0;
  while (nonzero_byte < mask.size() && !mask.test(nonzero_byte)) { ++nonzero_byte; }

  if (nonzero_byte != 0 && prev_visited_bytes != 0) {
    printf("[TL_MEMORY] Found first 1 in mask at index != 0 for recursive burst message call, will exit now\n");
    return;
  }

  char entry [2];
  size_t total_bytes = std::pow(2, tla->size());
  size_t visited_bytes = prev_visited_bytes;

  const char *data = tla->data().c_str();
//  printf("Data field: %s\n", data);
  for (size_t byte_idx = nonzero_byte + visited_bytes; visited_bytes < total_bytes; byte_idx++) {
    entry[0] = data[30  - 2 * (byte_idx % 16)];
    entry[1] =  data[31 - 2 * (byte_idx % 16)];
    uint8_t value = std::stoul(entry, NULL, 16);

//    printf("Storing value %u\n", value);
    p->get_mmu()->store_uint8(tla->address() + byte_idx, value);

    ++visited_bytes;

    if (visited_bytes == total_bytes) {
      // Done storing, send AccessAck
      verif::TLD tld;
      constructAccessAck(tla->source(), tla->size(), &tld);
      google::protobuf::util::SerializeDelimitedToOstream(tld, tld_ofs);
    } else if (byte_idx % 16 == 15) {
      tla_ifs->open((std::string(cosim_path) + "/TLAPipe").c_str(), std::fstream::in | std::fstream::binary);

      if (!tla->ParseFromIstream(tla_ifs)) {
        printf("[TL MEMORY] Failed to parse TL-A message from input stream \n");
      }

      tla_ifs->close();
      putFullData(tla, visited_bytes);
    }
  }
}

void tl_memory_t::putPartialData(verif::TLA *tla, size_t prev_visited_bytes) {
  if (tla->param() != 0 || tla->corrupt() != 0 || (tla->address() & ((1 << tla->size()) - 1)) != 0) {
    printf("[TL_MEMORY] Doing nothing for malformed TL-A: \n");
    printf("%s \n", tla->DebugString().c_str());
    return;
  }

//  printf("Processing putFullData\n");

  std::bitset<16> mask(tla->mask());

  char entry [2];
  size_t total_bytes = std::pow(2, tla->size());
  size_t visited_bytes = prev_visited_bytes;

  const char *data = tla->data().c_str();
//  printf("Data field: %s\n", data);
  for (size_t byte_idx = visited_bytes; visited_bytes < total_bytes; byte_idx++) {
    if (mask.test(byte_idx % 16)) {
      entry[0] = data[30 - 2 * (byte_idx % 16)];
      entry[1] = data[31 - 2 * (byte_idx % 16)];
      uint8_t value = std::stoul(entry, NULL, 16);

      //    printf("Storing value %u\n", value);
      p->get_mmu()->store_uint8(tla->address() + byte_idx, value);
    }

    ++visited_bytes;

    if (visited_bytes == total_bytes) {
      // Done storing, send AccessAck
      verif::TLD tld;
      constructAccessAck(tla->source(), tla->size(), &tld);
      google::protobuf::util::SerializeDelimitedToOstream(tld, tld_ofs);
    } else if (byte_idx % 16 == 15) {
      tla_ifs->open((std::string(cosim_path) + "/TLAPipe").c_str(), std::fstream::in | std::fstream::binary);

      if (!tla->ParseFromIstream(tla_ifs)) {
        printf("[TL MEMORY] Failed to parse TL-A message from input stream \n");
      }

      tla_ifs->close();
      putPartialData(tla, visited_bytes);
    }
  }
}

void tl_memory_t::get(verif::TLA *tla) {
  if (tla->param() != 0 || tla->corrupt() != 0 || (tla->address() & ((1 << tla->size()) - 1)) != 0) {
    printf("[TL_MEMORY] Doing nothing for malformed TL-A: \n");
    printf("%s \n", tla->DebugString().c_str());
    return;
  }

//  printf("[TL_MEMORY] Processing get\n");

  std::bitset<16> mask(tla->mask());

  // Start  at first non-zero entry of byte mask
  size_t nonzero_byte = 0;
  while (nonzero_byte < mask.size() && !mask.test(nonzero_byte)) { ++nonzero_byte; }

  if (nonzero_byte == mask.size()) {
    printf("[TL_MEMORY] No 1s found in write mask, exiting now\n");
    return;
  }

  // Load 2^(tla->size()) bytes, spill over into another message if too large
  char value[32];
  char entry[3];
  size_t total_bytes = std::pow(2, tla->size());
  size_t fetched_bytes = 0;
  for (size_t byte_idx = nonzero_byte; fetched_bytes < total_bytes; byte_idx++) {
    snprintf(entry, 3, "%02X", p->get_mmu()->load_uint8(((uint64_t) tla->address()) + byte_idx));
    //printf("Entry loaded %s for fetched byte: %lu at address %lx \n", entry, fetched_bytes, tla->address() + byte_idx);
    value[30 - 2 * (byte_idx % 16)] = entry[0];
    value[31 - 2 * (byte_idx % 16)] = entry[1];

    ++fetched_bytes;

    // If we just fetched the last byte or have filled up a message, we need to send a response with the data
    // In the case of a full message we also need to clear the value to keep constructing the next message
    if (fetched_bytes == total_bytes || byte_idx % 16 == 15) {
      //printf("Finished value: %s \n", value);
      verif::TLD tld;
      constructAccessAckData(tla->source(), tla->size(), value, &tld);
      google::protobuf::util::SerializeDelimitedToOstream(tld, tld_ofs);
      memset(value, 0, sizeof value);
    }
  }
}

void tl_memory_t::constructAccessAck(uint32_t source, uint32_t size, verif::TLD *tld) {
  tld->set_opcode(1);
  tld->set_param(0);
  tld->set_size(size);
  tld->set_source(source);
  tld->set_denied(false);
  tld->set_corrupt(false);
}
void tl_memory_t::constructAccessAckData(uint32_t source, uint32_t size, char* data, verif::TLD *tld) {
  tld->set_opcode(1);
  tld->set_param(0);
  tld->set_size(size);
  tld->set_source(source);
  tld->set_denied(false);
  tld->set_corrupt(false);
  tld->set_data(data, 32);
}