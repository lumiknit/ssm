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

def parseArgs value
  if value.is_a? Integer
    return [value]
  elsif value.is_a? Float
    return [value]
  elsif value.is_a? String
    return [value]
  elsif value.is_a? Array
    arr = []
    value.each do |v|
      arr += parseArgs v
    end
    return arr
  else
    puts "Error: Invalid file format"
    exit
  end
end

contents = YAML::load_file filename
lines = []
contents.each_with_index do |line, index|
  if line.is_a? String
    op = opcodes[line.upcase]
    if op == nil
      puts "Error: Invalid opcode #{line} at line #{index}"
      exit
    end
    lines << SSM::Line.new(op, [])
  elsif line.is_a? Hash
    line.each do |key, value|
      op = opcodes[key]
      if op == nil
        puts "Error: Invalid opcode #{key} at line #{index}"
        exit
      end
      args = parseArgs value
      lines << SSM::Line.new(op, args)
    end
  else
    puts "Error: Invalid file format"
    exit
  end
end

bytes = "".b

for line in lines
  bytes += line.to_bytes
end

File.binwrite "#{filename}.ssm", bytes