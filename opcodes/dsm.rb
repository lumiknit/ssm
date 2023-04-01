#!/usr/bin/env ruby
# -*- coding: utf-8 -*-

# dsm.rb
# Assembler for SSM assembly language
# Author: lumiknit

# See asm.rb comments for SSM assembly language syntax

require_relative 'spec'
$spec = SSM::spec

# --- Bytes unmarshal

def unmarshal_bytes bytes, filename="STR"
  size = bytes.length
  lines = []
  while bytes.length > 0
    p = size - bytes.length
    # Extract opcode
    o, rest = bytes.unpack "Ca*"
    bytes = rest

    # Get opcode 
    opcode = $spec.op o
    if opcode.nil?
      raise "Unknown opcode: #{o}"
    end

    # Get arguments
    args = []
    opcode.args.each do |arg|
      v, rest = arg.type.unpack bytes
      args << v
      bytes = rest
    end

    # Add line
    lines << {
      opcode: opcode,
      args: args,
      filename: filename,
      pos: p
    }
  end
  
  lines
end

def unmarshaled_to_s lines
  lines.map do |line|
    opcode = line[:opcode]
    args = line[:args]
    "#{opcode.name} #{args.map{|x| x.to_s}.join ', '}"
  end.join "\n"
end

# --- Find labels

def create_pos_map lines
  pos_map = {}
  lines.each do |line|
    pos_map[line[:pos]] = line
  end
  pos_map
end

def convert_labels! lines
  # Create pos_set
  pos_map = create_pos_map lines
  # Create labels
  pos_to_labels_map = {}
  errors = []
  lines.each do |line|
    opcode = line[:opcode]
    vals = line[:args]
    opcode.args.each_with_index do |arg, i|
      if arg.type.is_a? SSM::ArrayType and arg.type.elem_type.kind == "offset"
        vals[i].each_with_index do |offset, j|
          p = line[:pos] + offset
          if pos_map[p].nil?
            errors << "#{line[:filename]}:#{line[:pos]}: Offset #{offset} is incorrect offset"
          end
          if pos_to_labels_map[p].nil?
            pos_to_labels_map[p] = "L_#{pos_to_labels_map.length}"
          end
          vals[i][j] = pos_to_labels_map[p]
        end
      elsif arg.type.is_a? SSM::LitType and arg.type.type.kind == "offset"
        p = line[:pos] + vals[i]
        if pos_map[p].nil?
          errors << "#{line[:filename]}:#{line[:pos]}: Offset #{arg} is incorrect offset"
        end
        if pos_to_labels_map[p].nil?
          pos_to_labels_map[p] = "L_#{pos_to_labels_map.length}"
        end
        vals[i] = pos_to_labels_map[p]
      elsif arg.type.is_a? SSM::LitType and arg.type.type.kind == "magic"
        m = vals[i]
        mg = $spec.magic m
        if mg.nil?
          errors << "#{line[:filename]}:#{line[:pos]}: Magic #{m} is unknown"
        end
        vals[i] = mg.name
      end
    end
  end
  if errors.length > 0
    raise "Failed to convert labels:\n#{errors.join "\n"}"
  end
  pos_to_labels_map
end

def add_labels! lines
  pos_to_labels_map = convert_labels! lines
  lines.map do |line|
    if pos_to_labels_map[line[:pos]]
      line[:label] = pos_to_labels_map[line[:pos]]
    else
      line[:label] = nil
    end
  end
end

def labeled_to_s lines
  lines.map do |line|
    l = ""
    l += "#{line[:label]}:\n" if line[:label]
    l += "  #{line[:opcode].name}"
    line[:args].each_with_index do |arg, i|
      if i == 0
        l += " "
      else
        l += ","
      end
      if arg.is_a? Array
        if arg.length == 0
          l += "[]"
        else
          if arg[0].is_a? String
            l += arg.map{|x| x.to_s}.join ' '
          else
            b = ""
            arg.each do |x|
              if 32 <= x and x <= 126 and x != '"'.ord
                b += x.chr
              else
                if b.length > 0
                  l += '"' + b + '"'
                  b = ""
                end
                l += "0x#{x.to_s(16)}"
              end
            end
            l += '"' + b + '"' if b.length > 0
          end
        end
        
      else
        l += arg.to_s
      end
    end
    l
  end.join "\n"
end

# --- Change format using type

def change_value_format lines
  lines.each do |line|
    opcode = line[:opcode]
    args = line[:args]
    opcode.args.each_with_index do |arg, i|
      args[i] = arg.type.format args[i]
    end
  end
end

# --- CLI

def deassemble filename
  # Read file
  bytes = File.binread filename

  # Unmarshal
  lines = unmarshal_bytes bytes
  add_labels! lines
  puts labeled_to_s lines
end

# Get filename
if ARGV.length != 1
  puts "Usage: #{$0} <asm file>"
  exit 1
end
deassemble ARGV[0]
