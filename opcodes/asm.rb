#!/usr/bin/env ruby

# SSM Opcode Assembler
# @file asm.rb
# @author lumiknit (aasr4r4@gmail.com)
# @copyright Copyright (c) 2023 lumiknit
# @copyright This file is part of ssm.

require './op'
opcodes = SSM.read_opcodes_yaml "opcodes.yaml"

# Main
filename = ARGV[0]
if filename.nil?
  puts "Usage: ruby asm.rb <filename>"
  exit
end

# Read file
require 'yaml'
contents = YAML::load_file filename
lines = []
contents.each_with_index do |line, index|
  if line.is_a? String
    lines << SSM::Line.new(line, index)
  else

  unless line.is_a? Array or line.length != 1
    raise "#{filename}:#{line}: Each line must be an array of length 1"
  end
  line.each do |op, args|
    puts "#{op} #{args}"
  end
end
