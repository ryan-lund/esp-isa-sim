require_rv64;
require_fp;
softfloat_roundingMode = VFRM;
WRITE_SFRD(ui64_to_f32(RS1).v);
set_fp_exceptions;
