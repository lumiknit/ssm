

require 'json'

module Arg
  # Raw Type (Int, Uint, Float)
  class NumType
    attr_reader :size, :pack_directive

    def to_s
      "Unknown"
    end

    def to_asmtype
      "unk"
    end

    def to_ctype
      "unknown_t"
    end
  end

  class IntLitType < NumType
    def initialize size
      case size
      when 1
        @pack_directive = "c"
      when 2
        @pack_directive = "s<"
      when 4
        @pack_directive = "l<"
      when 8
        @pack_directive = "q<"
      else
        raise "Invalid size for IntRawType"
      end
      @size = size
    end

    def to_s
      "Int#{8 * @size}"
    end

    def to_asmtype
      "i#{8 * @size}"
    end

    def to_ctype
      "int#{8 * @size}_t"
    end
  end

  class UintLitType < NumType
    def initialize size
      case size
      when 1
        @pack_directive = "C"
      when 2
        @pack_directive = "S<"
      when 4
        @pack_directive = "L<"
      when 8
        @pack_directive = "Q<"
      else
        raise "Invalid size for UintRawType"
      end
      @size = size
    end

    def to_s
      "Uint#{8 * @size}"
    end

    def to_asmtype
      "u#{8 * @size}"
    end

    def to_ctype
      "uint#{8 * @size}_t"
    end
  end

  class FloatLitType < NumType
    def initialize size
      case size
      when 4
        @pack_directive = "e"
        @ctype = "float"
      when 8
        @pack_directive = "E"
        @ctype = "double"
      else
        raise "Invalid size for FloatRawType"
      end
      @size = size
    end

    def to_s
      "Float#{8 * @size}"
    end

    def to_asmtype
      "f#{8 * @size}"
    end

    def to_ctype
      @ctype
    end
  end

  # Type

  class Type
    def self.from_hash hash
      t = hash["type"]
      b = hash["bytes"]
      raise "Expect String for type" unless t.is_a? String
      raise "Expect Integer for bytes" unless b.is_a? Integer
      case t
      when "int"
        LitType.new IntLitType.new b
      when "uint"
        LitType.new UintLitType.new b
      when "float"
        LitType.new FloatLitType.new b
      when "offset"
        OffsetType.new b
      when "magic"
        MagicType.new b
      when "array"
        et = hash["elemType"]
        eb = hash["elemBytes"]
        raise "Expect String for elem_type" unless et.is_a? String
        raise "Expect Integer for elem_bytes" unless eb.is_a? Integer
        case et
        when "int"
          ArrayType.new b, LitType.new(IntLitType.new(eb))
        when "uint"
          ArrayType.new b, LitType.new(UintLitType.new(eb))
        when "float"
          ArrayType.new b, LitType.new(FloatLitType.new(eb))
        when "offset"
          ArrayType.new b, OffsetType.new(eb)
        when "magic"
          ArrayType.new b, MagicType.new(eb)
        else
          raise "Unknown elem_type #{et}"
        end
      else
        raise "Unknown type #{t}"
      end
    end

    def self.from_hash_hash hhash
      h = {}
      hhash.each do |k, v|
        h[k] = from_hash v
      end
      h
    end
  end

  class ArrayType < Type
    attr_reader :lit_type, :elem_lit_type

    def initialize bytes, elem_lit_type
      @lit_type = UintLitType.new bytes
      @elem_lit_type = elem_lit_type
    end
    
    def to_s
      "[array #{@lit_type} => #{@elem_lit_type}]"
    end
  end

  class LitType < Type
    attr_reader :lit_type

    def initialize lit_type
      raise "Expect NumType for lit_type" unless lit_type.is_a? NumType
      @lit_type = lit_type
    end

    def to_s
      "[#{@lit_type}]"
    end

    def pack data
      data.pack @lit_type.pack_directive
    end
  end

  class OffsetType < Type
    attr_reader :lit_type

    def initialize bytes
      @lit_type = IntLitType.new bytes
    end

    def to_s
      "[offset #{@lit_type}]"
    end
  end

  class MagicType < Type
    attr_reader :lit_type

    def initialize bytes
      @lit_type = UintLitType.new bytes
    end

    def to_s
      "[magic #{@lit_type}]"
    end
  end
end

module Op
  # TODO: Add opcode parser
end

module Magic
  # TODO: Add magic parser
end