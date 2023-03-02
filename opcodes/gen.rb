#!/usr/bin/env ruby

# SSM Opcode Example
# @file gen.rb
# @author lumiknit (aasr4r4@gmail.com)
# @copyright Copyright (c) 2023 lumiknit
# @copyright This file is part of ssm.

require "./op.rb"

# Main

# Read file binary
file = File.binread "../ssm"

opcodes = SSM.read_opcodes_yaml "opcodes.yaml"
for i in 0..opcodes.count
    op = opcodes.get i
    puts op
end