// Generated by /opcode/cgen.rb
// If this file is modified, please add a comment


#ifndef SSM_OPS_H
#define SSM_OPS_H

#include <stdint.h>

typedef uint8_t ssmOp;
#define SSM_OP_NOP ((ssmOp)0)
#define SSM_OP_POP ((ssmOp)1)
#define SSM_OP_PUSH ((ssmOp)2)
#define SSM_OP_PUSHBP ((ssmOp)3)
#define SSM_OP_PUSHAP ((ssmOp)4)
#define SSM_OP_POPSET ((ssmOp)5)
#define SSM_OP_PUSHI ((ssmOp)6)
#define SSM_OP_PUSHF ((ssmOp)7)
#define SSM_OP_PUSHFN ((ssmOp)8)
#define SSM_OP_PUSHGLOBAL ((ssmOp)9)
#define SSM_OP_POPSETGLOBAL ((ssmOp)10)
#define SSM_OP_PUSHISLONG ((ssmOp)11)
#define SSM_OP_TUP ((ssmOp)12)
#define SSM_OP_PUSHTAG ((ssmOp)13)
#define SSM_OP_PUSHLEN ((ssmOp)14)
#define SSM_OP_PUSHELEM ((ssmOp)15)
#define SSM_OP_LONG ((ssmOp)16)
#define SSM_OP_POPSETBYTE ((ssmOp)17)
#define SSM_OP_PUSHLONGLEN ((ssmOp)18)
#define SSM_OP_PUSHBYTE ((ssmOp)19)
#define SSM_OP_JOIN ((ssmOp)20)
#define SSM_OP_SUBLONG ((ssmOp)21)
#define SSM_OP_LONGCMP ((ssmOp)22)
#define SSM_OP_APP ((ssmOp)23)
#define SSM_OP_RET ((ssmOp)24)
#define SSM_OP_RETAPP ((ssmOp)25)
#define SSM_OP_INTADD ((ssmOp)26)
#define SSM_OP_INTSUB ((ssmOp)27)
#define SSM_OP_INTMUL ((ssmOp)28)
#define SSM_OP_UINTMUL ((ssmOp)29)
#define SSM_OP_INTDIV ((ssmOp)30)
#define SSM_OP_UINTDIV ((ssmOp)31)
#define SSM_OP_INTMOD ((ssmOp)32)
#define SSM_OP_UINTMOD ((ssmOp)33)
#define SSM_OP_INTUNM ((ssmOp)34)
#define SSM_OP_INTSHL ((ssmOp)35)
#define SSM_OP_INTSHR ((ssmOp)36)
#define SSM_OP_UINTSHR ((ssmOp)37)
#define SSM_OP_INTAND ((ssmOp)38)
#define SSM_OP_INTOR ((ssmOp)39)
#define SSM_OP_INTXOR ((ssmOp)40)
#define SSM_OP_INTNEG ((ssmOp)41)
#define SSM_OP_INTLT ((ssmOp)42)
#define SSM_OP_INTLE ((ssmOp)43)
#define SSM_OP_FLOATADD ((ssmOp)44)
#define SSM_OP_FLOATSUB ((ssmOp)45)
#define SSM_OP_FLOATMUL ((ssmOp)46)
#define SSM_OP_FLOATDIV ((ssmOp)47)
#define SSM_OP_FLOATUNM ((ssmOp)48)
#define SSM_OP_FLOATLT ((ssmOp)49)
#define SSM_OP_FLOATLE ((ssmOp)50)
#define SSM_OP_EQ ((ssmOp)51)
#define SSM_OP_NE ((ssmOp)52)
#define SSM_OP_JMP ((ssmOp)53)
#define SSM_OP_BEZ ((ssmOp)54)
#define SSM_OP_BNE ((ssmOp)55)
#define SSM_OP_BTAG ((ssmOp)56)
#define SSM_OP_JTAG ((ssmOp)57)
#define SSM_OP_MAGIC ((ssmOp)58)
#define SSM_OP_XFN ((ssmOp)59)
#define SSM_OP_HEADER ((ssmOp)60)

typedef uint16_t ssmMagic;
#define SSM_MAGIC_NOP ((ssmMagic)0)
#define SSM_MAGIC_PTOP ((ssmMagic)1)
#define SSM_MAGIC_HALT ((ssmMagic)2)
#define SSM_MAGIC_NEWVM ((ssmMagic)3)
#define SSM_MAGIC_NEWPROCESS ((ssmMagic)4)
#define SSM_MAGIC_VMSELF ((ssmMagic)5)
#define SSM_MAGIC_VMPARENT ((ssmMagic)6)
#define SSM_MAGIC_DUP ((ssmMagic)7)
#define SSM_MAGIC_GLOBALC ((ssmMagic)8)
#define SSM_MAGIC_EXECUTE ((ssmMagic)9)
#define SSM_MAGIC_HALTED ((ssmMagic)10)
#define SSM_MAGIC_SENDMSG ((ssmMagic)11)
#define SSM_MAGIC_HASMSG ((ssmMagic)12)
#define SSM_MAGIC_RECVMSG ((ssmMagic)13)
#define SSM_MAGIC_EVAL ((ssmMagic)14)
#define SSM_MAGIC_FOPEN ((ssmMagic)15)
#define SSM_MAGIC_FCLOSE ((ssmMagic)16)
#define SSM_MAGIC_FFLUSH ((ssmMagic)17)
#define SSM_MAGIC_FREAD ((ssmMagic)18)
#define SSM_MAGIC_FWRITE ((ssmMagic)19)
#define SSM_MAGIC_FTELL ((ssmMagic)20)
#define SSM_MAGIC_FSEEK ((ssmMagic)21)
#define SSM_MAGIC_FEOF ((ssmMagic)22)
#define SSM_MAGIC_STDREAD ((ssmMagic)23)
#define SSM_MAGIC_STDWRITE ((ssmMagic)24)
#define SSM_MAGIC_STDERROR ((ssmMagic)25)
#define SSM_MAGIC_REMOVE ((ssmMagic)26)
#define SSM_MAGIC_RENAME ((ssmMagic)27)
#define SSM_MAGIC_TMPFILE ((ssmMagic)28)
#define SSM_MAGIC_READFILE ((ssmMagic)29)
#define SSM_MAGIC_WRITEFILE ((ssmMagic)30)
#define SSM_MAGIC_MALLOC ((ssmMagic)31)
#define SSM_MAGIC_FREE ((ssmMagic)32)
#define SSM_MAGIC_SRAND ((ssmMagic)33)
#define SSM_MAGIC_RAND ((ssmMagic)34)
#define SSM_MAGIC_ARG ((ssmMagic)35)
#define SSM_MAGIC_ENV ((ssmMagic)36)
#define SSM_MAGIC_EXIT ((ssmMagic)37)
#define SSM_MAGIC_SYSTEM ((ssmMagic)38)
#define SSM_MAGIC_PI ((ssmMagic)39)
#define SSM_MAGIC_E ((ssmMagic)40)
#define SSM_MAGIC_ABS ((ssmMagic)41)
#define SSM_MAGIC_SIN ((ssmMagic)42)
#define SSM_MAGIC_COS ((ssmMagic)43)
#define SSM_MAGIC_TAN ((ssmMagic)44)
#define SSM_MAGIC_ASIN ((ssmMagic)45)
#define SSM_MAGIC_ACOS ((ssmMagic)46)
#define SSM_MAGIC_ATAN ((ssmMagic)47)
#define SSM_MAGIC_ATAN2 ((ssmMagic)48)
#define SSM_MAGIC_EXP ((ssmMagic)49)
#define SSM_MAGIC_LOG ((ssmMagic)50)
#define SSM_MAGIC_LOG10 ((ssmMagic)51)
#define SSM_MAGIC_MODF ((ssmMagic)52)
#define SSM_MAGIC_POW ((ssmMagic)53)
#define SSM_MAGIC_SQRT ((ssmMagic)54)
#define SSM_MAGIC_CEIL ((ssmMagic)55)
#define SSM_MAGIC_FLOOR ((ssmMagic)56)
#define SSM_MAGIC_FABS ((ssmMagic)57)
#define SSM_MAGIC_FMOD ((ssmMagic)58)
#define SSM_MAGIC_CLOCK ((ssmMagic)59)
#define SSM_MAGIC_TIME ((ssmMagic)60)
#define SSM_MAGIC_CWD ((ssmMagic)61)
#define SSM_MAGIC_ISDIR ((ssmMagic)62)
#define SSM_MAGIC_ISFILE ((ssmMagic)63)
#define SSM_MAGIC_MKDIR ((ssmMagic)64)
#define SSM_MAGIC_RMDIR ((ssmMagic)65)
#define SSM_MAGIC_CHDIR ((ssmMagic)66)
#define SSM_MAGIC_FILES ((ssmMagic)67)
#define SSM_MAGIC_JOINPATH ((ssmMagic)68)
#define SSM_MAGIC_FFILOAD ((ssmMagic)69)
#define SSM_MAGIC_OS ((ssmMagic)70)
#define SSM_MAGIC_ARCH ((ssmMagic)71)
#define SSM_MAGIC_ENDIAN ((ssmMagic)72)
#define SSM_MAGIC_VERSION ((ssmMagic)73)

#endif
