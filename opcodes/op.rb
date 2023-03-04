# SSM Opcode List Library
# @file op.rb
# @author lumiknit (aasr4r4@gmail.com)
# @copyright Copyright (c) 2023 lumiknit
# @copyright This file is part of ssm.

module SSM
  # Types

  class ArgType
    def to_s; raise "Unimplemented!"; end
    def bytes str; raise "Unimplemented!"; end

    def pack value
      [value].pack @format
    end

    def unpack str
      str.unpack @format + "a*"
    end
  end

  class SignedInt < ArgType
    @@formats = {
      8 => "c<",
      16 => "s<",
      32 => "l<",
      64 => "q<",
    }
    def self.formats; @@formats; end

    attr_reader :bits, :bytes, :format, :ruby_class

    def initialize bytes
      @bytes = bytes
      @bits = bytes * 8
      @format = @@formats[@bits]
      @ruby_class = 0.class
      raise "Invalid bits #{bits} for signed int" unless @format
    end

    def to_s
      "i#{@bits}"
    end

    def bytes str
      @bytes
    end
  end

  class UnsignedInt < ArgType
    @@formats = {
      8 => "C<",
      16 => "S<",
      32 => "L<",
      64 => "Q<",
    }
    def self.formats; @@formats; end

    attr_reader :bits, :bytes, :format, :ruby_class

    def initialize bytes
      @bytes = bytes
      @bits = bytes * 8
      @format = @@formats[@bits]
      @ruby_class = 0.class
      raise "Invalid bits #{@bits} for unsigned int" unless @format
    end

    def to_s
      "u#{@bits}"
    end

    def bytes str
      @bytes
    end
  end

  class Float < ArgType
    @@formats = {
      32 => "e",
      64 => "E",
    }
    def self.formats; @@formats; end

    attr_reader :bits, :bytes, :format, :ruby_class

    def initialize bytes
      @bytes = bytes
      @bits = bytes * 8
      @format = @@formats[@bits]
      @ruby_class = (0.0).class
      raise "Invalid bits #{@bits} for float" unless @format
    end

    def to_s
      "f#{@bits}"
    end

    def bytes str
      @bytes
    end
  end

  class Bytes < ArgType
    attr_reader :lenBytes, :lenBits, :lenFormat, :ruby_class

    def initialize lenBytes
      @lenBytes = lenBytes
      @lenBits = lenBytes * 8
      @lenFormat = UnsignedInt.formats[@lenBits]
      @ruby_class = String
      raise "Invalid length bits #{@lenBits} for bytes" if @lenFormat == nil
    end

    def to_s
      "b#{@lenBits}"
    end

    def bytes str
      @lenBytes + str.unpack(@lenFormat)[0]
    end

    def pack value
      len = value.length
      lenStr = [len].pack(@lenFormat)
      lenStr + value
    end

    def unpack str
      len = str.unpack(@lenFormat)[0] + "a*"
      return str[@lenBytes, len]
    end
  end

  class Offset < SignedInt
    def to_s
      "o#{@bits}"
    end
  end

  class JumpTable < ArgType
    attr_reader :lenBytes, :lenBits, :lenFormat, :ruby_class
    
    def initialize lenBytes, offBytes
      @lenBytes = lenBytes
      @lenBits = lenBytes * 8
      @lenFormat = UnsignedInt.formats[@lenBits]
      raise "Invalid length bits #{@lenBits} for jump table" if @lenFormat == nil
      @offBytes = offBytes
      @offBits = offBytes * 8
      @offFormat = SignedInt.formats[@offBits]
      raise "Invalid offset bits #{@offBits} for jump table" if @offFormat == nil
      @ruby_class = Array
    end

    def to_s
      "j#{@lenBits}_#{@offBits}"
    end

    def bytes str
      @lenBytes + str.unpack(@lenFormat)[0] * @offBytes
    end

    def pack value
      len = value.length
      lenStr = [len].pack(@lenFormat)
      lenStr + value.map{|v| [v].pack(@offFormat)}.join
    end

    def unpack str
      len = str.unpack(@lenFormat)[0]
      return str[@lenBytes, len].unpack(@offFormat * len)
    end
  end

  @@i8 = SignedInt.new(1)
  @@i16 = SignedInt.new(2)
  @@i32 = SignedInt.new(4)
  @@i64 = SignedInt.new(8)
  @@u8 = UnsignedInt.new(1)
  @@u16 = UnsignedInt.new(2)
  @@u32 = UnsignedInt.new(4)
  @@u64 = UnsignedInt.new(8)
  @@f32 = Float.new(4)
  @@f64 = Float.new(8)
  @@b32 = Bytes.new(4)
  @@o16 = Offset.new(2)
  @@o32 = Offset.new(4)
  @@j16_32 = JumpTable.new(2, 4)

  @@arg_types = [
    # Signed integers
    @@i8, @@i16, @@i32, @@i64,
    # Unsigned integers
    @@u8, @@u16, @@u32, @@u64,
    # Floats
    @@f32, @@f64,
    # Bytes
    @@b32,
    # Offsets
    @@o16, @@o32,
    # Jump tables
    @@j16_32,
  ]

  @@arg_type_map = {}
  @@arg_types.each do |type|
    @@arg_type_map[type.to_s] = type
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
      "#{name}:#{type.to_s}"
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

  class Line
    attr_reader :op, :args

    def initialize op, args
      @op = op
      @args = args
      @args = [] if @args == nil
      # Check types
      if @op.args.length != @args.length
        raise "Invalid number of arguments for #{op.name}"
      end
      @op.args.each_with_index do |arg, i|
        unless @args[i].is_a? arg.type.ruby_class
          raise "The argument #{arg.name} must be a #{arg.type.ruby_class}"
        end
      end
    end

    def to_bytes
      result = [@op.byte].pack("C")
      @op.args.each_with_index do |arg, i|
        result += arg.type.pack @args[i]
      end
      result
    end

    def to_s
      "#{@op.name} #{@args.map(&:to_s).join(' ')}"
    end
  end

  # Opcode Table

  class OpTable
    attr_reader :array, :map, :magic_array, :magic_map

    def initialize ops, magics
      # Set ops
      @array = []
      @map = {}
      ops.each_with_index do |op, i|
        @array << op
        @map[op.name] = op
      end
      # Set magics
      @magic_array = magics
      @magic_map = {}
      magics.each_with_index do |magic, i|
        @magic_map[magic] = i
      end
    end

    def get index
      @array[index]
    end

    def [] name
      @map[name]
    end

    def count
      @array.length
    end

    def each
      @array.each do |op|
        yield op
      end
    end

    def parse_code str
      result = []
      while str.length > 0
        byte, left = str.unpack("C<a*")
        op = get byte
        raise "Invalid opcode #{byte}" unless op
        args = []
        op.args.each do |arg|
          argStr, left = left.unpack("#{arg.type.format}a*")
          args << argStr
        end
        result << Line.new(op, args)
        str = left
      end
    end
  end

  def self.read_opcodes_yaml filename
    require 'yaml'
    yaml = YAML.load_file filename
    opcodes = []
    magics = []
    yaml['opcodes'].each do |pair|
      pair.each do |op_name, args|
        if args.is_a? Array
          args = args.map do |a|
            arg = nil
            a.each do |name, type|
              begin
                argType = @@arg_type_map[type]
                arg = Arg.new name, argType
              rescue
                raise "Invalid argument type #{type} for #{name} in #{op_name}"
              end
            end
            arg
          end
        else
          args = []
        end
        op = Op.new op_name, opcodes.length, args
        opcodes << op
      end
    end
    yaml['magics'].each do |name|
      magics << name
    end
    OpTable.new opcodes, magics
  end
end