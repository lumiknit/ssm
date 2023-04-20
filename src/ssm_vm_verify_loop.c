// Generated by /opcode/cgen.rb
// If this file is modified, please add a comment


switch(op) {
case SSM_OP_NOP: {
  i += 1;
} break;
case SSM_OP_POP: {
  i += 3;
} break;
case SSM_OP_PUSH: {
  i += 3;
} break;
case SSM_OP_PUSHBP: {
  i += 3;
} break;
case SSM_OP_PUSHAP: {
  i += 3;
} break;
case SSM_OP_POPSET: {
  i += 3;
} break;
case SSM_OP_PUSHI: {
  i += 5;
} break;
case SSM_OP_PUSHF: {
  i += 5;
} break;
case SSM_OP_PUSHFN: {
  int32_t fn_off = read_int32_t(&c->bytes[i + 1]);
  if(i + fn_off < 0 || i + fn_off >= c->size)
    goto L_err_offset;
  mark[i + fn_off] |= M_FN_TARGET;
  i += 5;
} break;
case SSM_OP_PUSHGLOBAL: {
  uint32_t global = read_uint32_t(&c->bytes[i + 1]);
  if(global >= n_globals)
    goto L_err_global;
  i += 5;
} break;
case SSM_OP_POPSETGLOBAL: {
  uint32_t global = read_uint32_t(&c->bytes[i + 1]);
  if(global >= n_globals)
    goto L_err_global;
  i += 5;
} break;
case SSM_OP_PUSHISLONG: {
  i += 3;
} break;
case SSM_OP_TUP: {
  i += 5;
} break;
case SSM_OP_PUSHTAG: {
  i += 3;
} break;
case SSM_OP_PUSHLEN: {
  i += 3;
} break;
case SSM_OP_PUSHELEM: {
  i += 5;
} break;
case SSM_OP_LONG: {
  uint32_t bytes_len = read_uint32_t(&c->bytes[i + 1]);
  i += 5 + bytes_len * sizeof(uint8_t);
} break;
case SSM_OP_POPSETBYTE: {
  i += 3;
} break;
case SSM_OP_PUSHLONGLEN: {
  i += 3;
} break;
case SSM_OP_PUSHBYTE: {
  i += 3;
} break;
case SSM_OP_JOIN: {
  i += 1;
} break;
case SSM_OP_SUBLONG: {
  i += 1;
} break;
case SSM_OP_LONGCMP: {
  i += 1;
} break;
case SSM_OP_APP: {
  i += 3;
  if(i % 2 != 0) goto L_err_right_aligned;
} break;
case SSM_OP_RET: {
  i += 3;
} break;
case SSM_OP_RETAPP: {
  i += 3;
  if(i % 2 != 0) goto L_err_right_aligned;
} break;
case SSM_OP_INTADD: {
  i += 1;
} break;
case SSM_OP_INTSUB: {
  i += 1;
} break;
case SSM_OP_INTMUL: {
  i += 1;
} break;
case SSM_OP_UINTMUL: {
  i += 1;
} break;
case SSM_OP_INTDIV: {
  i += 1;
} break;
case SSM_OP_UINTDIV: {
  i += 1;
} break;
case SSM_OP_INTMOD: {
  i += 1;
} break;
case SSM_OP_UINTMOD: {
  i += 1;
} break;
case SSM_OP_INTUNM: {
  i += 1;
} break;
case SSM_OP_INTSHL: {
  i += 1;
} break;
case SSM_OP_INTSHR: {
  i += 1;
} break;
case SSM_OP_UINTSHR: {
  i += 1;
} break;
case SSM_OP_INTAND: {
  i += 1;
} break;
case SSM_OP_INTOR: {
  i += 1;
} break;
case SSM_OP_INTXOR: {
  i += 1;
} break;
case SSM_OP_INTNEG: {
  i += 1;
} break;
case SSM_OP_INTLT: {
  i += 1;
} break;
case SSM_OP_INTLE: {
  i += 1;
} break;
case SSM_OP_FLOATADD: {
  i += 1;
} break;
case SSM_OP_FLOATSUB: {
  i += 1;
} break;
case SSM_OP_FLOATMUL: {
  i += 1;
} break;
case SSM_OP_FLOATDIV: {
  i += 1;
} break;
case SSM_OP_FLOATUNM: {
  i += 1;
} break;
case SSM_OP_FLOATLT: {
  i += 1;
} break;
case SSM_OP_FLOATLE: {
  i += 1;
} break;
case SSM_OP_EQ: {
  i += 1;
} break;
case SSM_OP_NE: {
  i += 1;
} break;
case SSM_OP_JMP: {
  int32_t off = read_int32_t(&c->bytes[i + 1]);
  if(i + off < 0 || i + off >= c->size)
    goto L_err_offset;
  mark[i + off] |= M_JMP_TARGET;
  i += 5;
} break;
case SSM_OP_BEZ: {
  int16_t off = read_int16_t(&c->bytes[i + 1]);
  if(i + off < 0 || i + off >= c->size)
    goto L_err_offset;
  mark[i + off] |= M_JMP_TARGET;
  i += 3;
} break;
case SSM_OP_BNE: {
  int16_t off = read_int16_t(&c->bytes[i + 1]);
  if(i + off < 0 || i + off >= c->size)
    goto L_err_offset;
  mark[i + off] |= M_JMP_TARGET;
  i += 3;
} break;
case SSM_OP_BTAG: {
  int16_t off = read_int16_t(&c->bytes[i + 3]);
  if(i + off < 0 || i + off >= c->size)
    goto L_err_offset;
  mark[i + off] |= M_JMP_TARGET;
  i += 5;
} break;
case SSM_OP_JTAG: {
  uint32_t jmps_len = read_uint32_t(&c->bytes[i + 1]);
  for(size_t jmps_i = 0; jmps_i < jmps_len; jmps_i++) {
    int32_t jmps_elem = read_int32_t(&c->bytes[i + 1 + 4 + jmps_i * sizeof(int32_t)]);
  if(i + jmps_elem < 0 || i + jmps_elem >= c->size)
    goto L_err_offset;
  mark[i + jmps_elem] |= M_JMP_TARGET;
  }
  i += 5 + jmps_len * sizeof(int32_t);
} break;
case SSM_OP_MAGIC: {
  uint16_t magic = read_uint16_t(&c->bytes[i + 1]);
  if(magic >= 74)
    goto L_err_magic;
  i += 3;
} break;
case SSM_OP_XFN: {
  mark[i] |= M_X_FN;
  if(i % 2 != 0) goto L_err_left_aligned;
  i += 5;
} break;
case SSM_OP_HEADER: {
  i += 17;
} break;
default: goto L_err_op;
}
