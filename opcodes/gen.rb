#!/usr/bin/env ruby

require 'yaml'

# Types
class ArgType
  attr_reader :kind, :bits

  @@kinds = {
    "i" => "int",
    "u" => "unsigned",
    "f" => "float",
    "b" => "bytes",
    "o" => "offset",
  }

  def initialize kind, bits
    kind = kind.downcase
    raise "Invalid kind #{kind}" if !@@kinds.has_key? kind
    raise "Invalid bits #{bits}" if bits % 8 != 0
    @kind = kind
    @bits = bits
  end

  def self.from_s str
    result = /([A-Za-z]+)([0-9]+)/.match str
    raise "Invalid type #{str}" if result == nil
    ArgType.new result[1], result[2].to_i
  end

  def to_s
    "#{kind}#{bits}"
  end

  def bytes
    bits / 8
  end
end

# Arguments
class Arg
  attr_reader :name, :type

  def initialize name, type
    raise "Invalid name #{name}" unless name.is_a? String
    raise "Invalid type #{type}" unless type.is_a? ArgType
    @name = name.downcase
    @type = type
  end

  def to_s
    "#{name}:#{type}"
  end
end

# Opcodes
class Op
  attr_reader :name, :byte, :args

  def initialize name, byte, args
    @name = name.upcase
    @byte = byte
    @args = args
  end

  def to_s
    "[#{byte}] #{name} #{args.map(&:to_s).join(' ')}"
  end
end

class Opcodes
  def initialize opcodes
    @opcode_array = []
    @opcode_map = {}
    opcodes.each_with_index do |op, i|
      @opcode_array << op
      @opcode_map[op.name] = op
    end
  end

  def get index
    @opcode_array[index]
  end

  def [] name
    @opcode_map[name]
  end

  def each
    @opcode_array.each do |op|
      yield op
    end
  end

  def self.from_yaml_file filename
    yaml = YAML.load_file filename
    opcodes = []
    yaml['opcodes'].each do |pair|
      pair.each do |name, args|
        if args.is_a? Array
          args = args.map do |a|
            arg = nil
            a.each do |name, type|
              argType = ArgType.from_s type
              arg = Arg.new name, argType
            end
            arg
          end
        else
          args = []
        end
        op = Op.new name, opcodes.length, args
        opcodes << op
        puts op
      end
    end
    Opcodes.new opcodes
  end
end

# Main

opcodes = Opcodes::from_yaml_file "opcodes.yaml"
puts opcodes