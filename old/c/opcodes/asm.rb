#!/usr/bin/env ruby
# -*- coding: utf-8 -*-

# asm.rb
# Assembler for SSM assembly language
# Author: lumiknit

# SSM assembly:
# Every (non-empty) line must be:
# [LABEL:] [OPCODE [ARG1, [ARG2, ...]]] [; COMMENT]
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

require_relative 'spec'
$spec = SSM.spec

# --- Line parser

def parse_arg str
  arg = []
  loop do
    # Pass spaces
    /^\s*(?<rest>.*)$/ =~ str
    str = rest

    # Check float
    if /^\s*(?<n>[-+]?[0-9]+\.[0-9]*)\s*(?<rest>.*)$/ =~ str
      str = rest
      arg << n.to_f
    # Check integer (all casese of hex, oct, bin, dec)
    elsif /^\s*(?<sign>[-+]?)(?<num_part>0x[0-9a-fA-F]+|0o[0-7]+|0b[0-1]+|[0-9]+)\s*(?<rest>.*)$/ =~ str
      str = rest
      # Check base and convert
      if /^0x(?<num>.+)$/ =~ num_part
        n = num.to_i 16
      elsif /^0o(?<num>.+)$/ =~ num_part
        n = num.to_i 8
      elsif /^0b(?<num>.+)$/ =~ num_part
        n = num.to_i 2
      else
        n = num_part.to_i
      end
      n *= -1 if sign == '-'
      arg << n
    # Check bytes
    elsif /^\s*"(?<s>[^"]*)"\s*(?<rest>.*)$/ =~ str
      str = rest
      # Convert to bytes
      arg << s
    elsif /^\s*'(?<s>[^']*)'\s*(?<rest>.*)$/ =~ str
      str = rest
      # Convert to bytes
      arg << s
    # Check id
    elsif /^\s*(?<id>[-_a-zA-Z0-9.]+)\s*(?<rest>.*)$/ =~ str
      str = rest
      arg << id
    else
      break
    end
  end
  return arg, str
end

def unmarshal_line str
  line = nil
  # First check label
  /^\s*(?<label>[-_a-zA-Z0-9.]+)\s*:\s*(?<rest>.*)$/ =~ str
  if label
    str = rest
    line = {label: label}
  end
  # Then, check opcode
  /^\s*(?<opcode>[-_a-zA-Z0-9.]+)\s*(?<rest>.*)$/ =~ str
  if opcode
    # Check opcode is valid
    unless $spec.op opcode
      raise "Unknown opcode: #{opcode}"
    end
    # Move to next str
    str = rest
    # Check args
    args = []
    loop do
      arg, str = parse_arg str
      unless arg.empty?
        args << arg
        # If there's no comma, break
        break unless /^\s*,\s*(?<rest>.*)$/ =~ str
        str = rest
      else
        break
      end
    end
    line = {} if line.nil?
    line[:opcode] = opcode
    line[:args] = args
  end
  # Check if it's empty line
  unless /^\s*(;.+)?$/ =~ str
    raise "Expect end of line, but got: #{str}"
  end
  line
end

def unmarshal_lines str, filename="STR"
  errors = []
  lines = []
  str.each_line.with_index(1) do |line, index|
    begin
      ul = unmarshal_line line
      if ul
        ul[:line] = index
        ul[:filename] = filename
        lines << ul
      end
    rescue ScriptError, StandardError => e
      raise e
    rescue => e
      errors << "#{filename}:#{index}: #{line.strip}\n  #{e}"
    end
  end
  if errors.empty?
    return lines
  else
    raise "Failed to unmarshal:\n#{errors.join "\n"}"
  end
end

def unmarshaled_lines_to_s lines
  ls = []
  lines.each do |line|
    l = ""
    if line[:label]
      l += "#{line[:label]}: "
    end
    if line[:opcode]
      l += "#{line[:opcode]}"
      if line[:args]
        l += " #{line[:args].map{|arg| arg.join ' '}.join ','}"
      end
    end
    ls << l
  end
  ls.join "\n"
end

# --- Check opcode byte index
def refine_lines lines
  label_table = {}
  new_lines = []
  p = 0
  lines.each do |line|
    # Handling for label
    if line[:label]
      if label_table[line[:label]]
        raise "Failed to refine:\n#{line[:filename]}:#{line[:line]}: Label #{line[:label]} is already defined"
      end
      label_table[line[:label]] = {
        filename: line[:filename],
        line: line[:line],
        pos: p,
        index: new_lines.length,
      }
    end

    # Get opcode
    op = $spec.op line[:opcode]
    next if op.nil?

    # Check args length
    if line[:args].length != op.args.length
      raise "Failed to refine:\n#{line[:filename]}:#{line[:line]}: Expect #{op.args.length} args for #{op.name}, but got #{line[:args].length}"
    end

    # Refine args
    off = 1
    new_args = []
    line[:args].length.times do |i|
      arg = op.args[i]
      val = line[:args][i]

      if arg.type.is_a? SSM::LitType
        # val's length must be 1
        if val.length != 1
          raise "Failed to refine:\n#{line[:filename]}:#{line[:line]}: #{arg} requires singleton, but got #{val}"
        end

        v = val[0]

        if arg.type.type.kind == "magic"
          # Check magic
          m = $spec.magic v
          if m.nil?
            raise "Failed to refine:\n#{line[:filename]}:#{line[:line]}: Unknown magic #{v}"
          end
          # If magic is valid, convert to magic index
          v = m.index
        elsif arg.type.type.kind == "offset"
          # In this case, we need labels
          unless v.is_a? String
            raise "Failed to refine:\n#{line[:filename]}:#{line[:line]}: #{arg.name} requires labels, but got #{v}"
          end
        else
          # val's type must be same as arg's type
          begin
            arg.type.type.check_val v
          rescue => e
            raise "Failed to refine:\n#{line[:filename]}:#{line[:line]}: #{arg.name} requires #{arg.type}, but got #{val}\n  #{e}"
          end
        end

        # Push val
        new_args << v

        # Update offset
        off += arg.type.type.bytes

      elsif arg.type.is_a? SSM::ArrayType
        et = arg.type.elem_type
        off += arg.type.len_type.bytes
        if et.kind == "uint" and et.bytes == 1
          # Convert each elements into bytes
          bytes = []
          val.each do |v|
            if v.is_a? String
              bytes += v.bytes
            elsif v.is_a? Integer
              unless 0 <= v and v <= 255
                raise "Failed to refine:\n#{line[:filename]}:#{line[:line]}: #{arg.name} requires uint8, but got #{v}"
              end
              bytes << v
            else
              raise "Failed to refine:\n#{line[:filename]}:#{line[:line]}: #{arg.name} requires bytes, but got #{v}"
            end
          end

          # Push bytes
          new_args << bytes

          # Update offset
          off += bytes.length
        elsif et.kind == "offset"
          # In this case, we need labels
          labels = []
          val.each do |v|
            if v.is_a? String
              labels << v
            else
              raise "Failed to refine:\n#{line[:filename]}:#{line[:line]}: #{arg.name} requires labels, but got #{v}"
            end
          end

          # Push labels
          new_args << labels

          # Update offset
          off += labels.length * et.bytes
        else
          raise "Failed to refine:\n#{line[:filename]}:#{line[:line]}: Unknown array elem type: #{et}"
        end
      else
        raise "Failed to refine:\n#{line[:filename]}:#{line[:line]}: Unknown arg type: #{arg.type}"
      end
    end
    # Push to newline
    new_lines << {
      opcode: op,
      args: new_args,
      filename: line[:filename],
      line: line[:line],
      pos: p,
    }
    p += off
  end
  return new_lines, label_table
end

def refined_to_s lines, labels
  ls = []
  ls << "; Labels:"
  labels.each do |name, label|
    ls << ";   #{name}: idx=#{label[:index]} pos=#{label[:pos]} (#{label[:filename]}:#{label[:line]})"
  end
  ls << ";"
  ls << "; Opcodes:"
  lines.each do |line|
    l = "#{line[:pos]}: #{line[:opcode].name}"
    line[:args].each.with_index do |arg, idx|
      if idx > 0
        l += ","
      else
        l += " "
      end
      if arg.is_a? Array
        l += "(#{arg.length})"
        l += "[" + arg.map{|v| v.to_s}.join(",") + "]"
      else
        l += arg.to_s
      end
    end
    ls << l
  end
  ls.join "\n"
end

# --- Convert label string to offset

def calculate_offset line, labels, lit_type, label_name
  label = labels[label_name]
  if label.nil?
    raise "Failed to translate labels:\n#{line[:filename]}:#{line[:line]}: Unknown label #{label_name}"
  end
  off = label[:pos] - line[:pos]
  begin
    lit_type.check_val off
  rescue => e
    raise "Failed to translate labels:\n#{line[:filename]}:#{line[:line]}: Offset #{off} is out of range for #{lit_type}\n  #{e}"
  end
  off
end

def translate_line! line, labels
  # Get opcode
  op = line[:opcode]
  line[:args].each.with_index do |val, idx|
    arg = op.args[idx]
    # If arg is offset, translate
    if arg.type.is_a? SSM::ArrayType and arg.type.elem_type.kind == "offset"
      val.length.times do |i|
        val[i] = calculate_offset line, labels, arg.type.elem_type, val[i]
      end
    elsif arg.type.is_a? SSM::LitType and arg.type.type.kind == "offset"
      line[:args][idx] = calculate_offset line, labels, arg.type.type, val
    end
  end
end

def translate_labels! lines, labels
  new_lines = []
  lines.each do |line|
    translate_line! line, labels
  end
  new_lines
end

def lines_to_s lines
  ls = []
  lines.each do |line|
    l = "#{line[:pos]}: #{line[:opcode].name}"
    line[:args].each.with_index do |arg, idx|
      if idx > 0
        l += ","
      else
        l += " "
      end
      if arg.is_a? Array
        l += "(#{arg.length})"
        l += "[" + arg.map{|v| v.to_s}.join(",") + "]"
      else
        l += arg.to_s
      end
    end
    ls << l
  end
  ls.join "\n"
end

# --- Marshal

def marshal_lines lines
  bytes = "".b
  lines.each do |line|
    o = line[:opcode]
    # First, write opcode as u8
    bytes += [o.index].pack("C")
    # Then, add args
    o.args.each.with_index do |arg, idx|
      v = line[:args][idx]
      bytes += arg.type.pack v
    end
  end
  bytes
end

# --- CLI

def assemble filename
  # Output file
  # Trim and append .ssm
  outname = filename.sub(/\.[^.]+$/, "") + ".ssm"

  # Read file
  puts "[INFO] Read #{filename}..."
  str = File.read filename

  # Unmarshal
  puts "[INFO] Converting sources into lines..."
  lines = unmarshal_lines str, filename
  puts "[INFO]   #lines = #{lines.length}"
  puts "[INFO] Refining labels..."
  lines, labels = refine_lines lines
  puts "[INFO] Translating labels..."
  translate_labels! lines, labels

  # Marshal
  puts "[INFO] Pack into bytes..."
  bytes = marshal_lines lines
  puts "[INFO] Write to #{outname}..."
  puts "       Size: #{bytes.length} bytes"
  File.write outname, bytes
end

# Get filename
if ARGV.length != 1
  puts "Usage: #{$0} <asm file>"
  exit 1
end
assemble ARGV[0]