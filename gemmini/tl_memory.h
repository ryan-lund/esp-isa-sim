#ifndef _TL_MEMORY_H
#define _TL_MEMORY_H

#include "../riscv/mmu.h"
#include "TL.pb.h"
#include <unistd.h>
#include <fstream>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#endif
class tl_memory_t
{
public:
    tl_memory_t() {}
    ~tl_memory_t() {}
    static void* threaded_run(void* tl_memory) {
      ((tl_memory_t *) tl_memory)->run();
      return 0;
    }

    void run();
    void debug_run(verif::TLA *tla);
    void set_processor(processor_t* _p) { p = _p; }
    void set_tla_ifs(std::fstream* _tla_ifs) {tla_ifs = _tla_ifs; }
    void set_tld_ofs(std::fstream* _tld_ofs) {tld_ofs = _tld_ofs; }
    void set_cosim_path(const char* _cosim_path) {cosim_path = _cosim_path; }

private:
    processor_t* p;
    std::fstream* tla_ifs;
    std::fstream* tld_ofs;
    const char* cosim_path;

    void processTLA(verif::TLA *tla);
    void putFullData(verif::TLA *tla, size_t prev_visited_bytes = 0);
    void putPartialData(verif::TLA *tla, size_t prev_visited_bytes = 0);
    void get(verif::TLA *tla);
    void constructAccessAck(uint32_t source, uint32_t size, verif::TLD *tld);
    void constructAccessAckData(uint32_t source, uint32_t size, char* data, verif::TLD *tld);
};