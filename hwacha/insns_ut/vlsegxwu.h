if(VPRED) {
  for (uint32_t j=0; j<INSN_VSEG; j++){
    UT_WRITE_XPR(UTIDX, INSN_VRD+j, MMU.load_uint32(SRS1 + RS2));
  }
}
