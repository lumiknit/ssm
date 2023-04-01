# asm.rb
# Assembler for SSM assembly language
# Author: lumiknit

# SSM assembly:
# Every (non-empty) line must be:
# [[LABEL:] OPCODE [ARG1, [ARG2, ...]]] [; COMMENT]
# ([...] means it's optional)
# Notes:
# - LABEL, OPCODE, ARGS are case insensitive.
# - All identifier must be [-_a-zA-Z0-9.]+
# - ARG can be:
#   - Int (e.g. 123, -42, 0x32, 0o32, 0b1010)
#   - Uint (e.g. 123, 0x32, 0o32, 0b1010)
#   - Float (3.141592)
#   - Bytes
#     - Both of quoted string & int can be mixed
#     - e.g. "Hello, World" 32 65 "!" 0
#   - Label (e.g. foo, bar, foo.bar)
#   - JumpTable (label array) (list labels with spaces) (e.g. L_1 L_2 ...)

require './spec'
spec = Spec::spec