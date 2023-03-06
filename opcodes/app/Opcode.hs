module Main where

import Data.List

type Opcode = (String, [ArgType])

{-
# -- NO OP
- NOP: [] # do nothing

# -- SSM Header/Footer
- HEADER: # code chunk header
  - token: u32
  - prevglobal: u32
  - newglobal: u32
- HALT: [] # halt the program

# -- STACK
- POP: # pop n elements from stack
  - n: u16 
- PUSH: # push [sp + idx]
  - idx: u16
- PUSHBP: # push [bp - idx]
  - idx: u16
- PUSHAP: # push [bp + idx]
  - idx: i16
- POPSET: # set [sp + idx] to [sp]
  - idx: u16

# -- LITERAL
- PUSHI: # push signed int
  - val: i32
- PUSHF: # push float
  - val: f32

# -- FUNCTION
- PUSHFN: # push function pointer
  - fn: o32

# -- GLOBAL
- PUSHGLOBAL: # push global variable
  - idx: u32
- POPSETGLOBAL: # pop and set to global variable
  - idx: u32

# -- TUPLE
- PUSHISLONG: # push islong flag at [idx]
  - idx: u16

# -- SHORT TUPLE
- TUP: # make a new tup using [sp .. sp + len]
  - tag: u16
  - len: u16
- TAG: # make a new tag-only tup without len
  - tag: u8
- PUSHTAG: # push a tag at [idx]
  - idx: u16
- PUSHLEN: # push a short tuple length at [idx]
  - idx: u16
- PUSHELEM: # push an element at [idx] at [jdx]
  - idx: u16
  - jdx: u16

# -- LONG TUPLE
- LONG: # push a long tuple
  - bytes: b32
- PACK: # pack elements into bytes
  - len: u32
- SETBYTE: # set a byte at [idx] at [jdx]
  - idx: u16
  - jdx: u16
- PUSHLONGLEN: # push a long tuple length at [idx]
  - idx: u16
- PUSHBYTE: # push a byte at [idx]
  - idx: u16
- JOIN: [] # join long tuple elements in a short tuple
- SUBLONG: [] # push a subtuple of a long tuple
- LONGCMP: [] # compare two long tuples

# -- CALL
- APP: # call function at [bp]
  - argc: u16
- RET: # return from function
  - idx: u16
- RETAPP: # tail call function at [bp]
  - argc: u16

# -- INT ARITHMETIC
- INTADD: [] # add two elements on top of stack
- INTSUB: [] # subtract two elements on top of stack
- INTMUL: [] # multiply two elements on top of stack
- UINTMUL: [] # multiply two elements on top of stack
- INTDIV: [] # divide two elements on top of stack
- UINTDIV: [] # divide two elements on top of stack
- INTMOD: [] # modulo two elements on top of stack
- UINTMOD: [] # modulo two elements on top of stack
- INTUNM: [] # unary minus

- INTSHL: [] # shift left two elements on top of stack
- INTSHR: [] # shift right two elements on top of stack
- UINTSHR: [] # shift right arithmetic two elements on top of stack
- INTAND: [] # bitwise and two elements on top of stack
- INTOR: [] # bitwise or two elements on top of stack
- INTXOR: [] # bitwise xor two elements on top of stack
- INTNEG: [] # bitwise not two elements on top of stack

- INTLT: [] # compare two elements on top of stack
- INTLE: [] # compare two elements on top of stack

# -- FLOAT ARITHMETIC
- FLOATADD: [] # add two elements on top of stack
- FLOATSUB: [] # subtract two elements on top of stack
- FLOATMUL: [] # multiply two elements on top of stack
- FLOATDIV: [] # divide two elements on top of stack
- FLOATUNM: [] # unary minus

- FLOATLT: [] # compare two elements on top of stack
- FLOATLE: [] # compare two elements on top of stack

# -- COMPARISON
- EQ: [] # compare two elements on top of stack
- NE: [] # compare two elements on top of stack

# -- BRANCH

- JMP: # jump to offset
  - offset: o32
- BEQ: # branch if not equal
  - offset: o32
- BNE: # branch if not equal
  - offset: o32
- BTAG: # branch if tag is
  - tag: u16
  - offset: o32
- JTAG: # jump table by tag
  - offsets: j16_32

# -- MAGIC
- MAGIC:
  - instr: m16

# -- LITERAL MARKER
- XFN: # function
  - argc: u16
  - stackreq: u32
-}

opcodes :: [Opcode]
opcodes = [
  ("NOP", []),
  ("HEADER", [u32, u32, u32]),
  ("HALT", []),
  ("POP", [u16])
]