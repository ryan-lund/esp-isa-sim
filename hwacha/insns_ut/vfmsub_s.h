require_fp;
softfloat_roundingMode = VFRM;
WRITE_SFRD(f32_mulAdd(f32(FRS1), f32(FRS2), f32(f32(FRS3).v ^ F32_SIGN)).v);
set_fp_exceptions;
