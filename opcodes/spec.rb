# -*- coding: utf-8 -*-

# spec.rb: SSM Spec Helper Classes
# Author: lumiknit

require 'json'

module SSM

  # --- Argument Spec

  def type_kind_data
    
  end

  # Raw Type (Int, Uint, Float)
  class NumType
    attr_reader :bytes, :kind, :pack_directive, :asm, :ctype

    @@type_kinds = {
      "int" => {
        name: "IntNumType",
        sized: {
          1 => { directive: "c", asm: "i8", ctype: "int8_t" },
          2 => { directive: "s<", asm: "i16", ctype: "int16_t" },
          4 => { directive: "l<", asm: "i32", ctype: "int32_t" },
          8 => { directive: "q<", asm: "i64", ctype: "int64_t" }
        }
      },
      "uint" => {
        name: "UintNumType",
        sized: {
          1 => { directive: "C", asm: "u8", ctype: "uint8_t" },
          2 => { directive: "S<", asm: "u16", ctype: "uint16_t" },
          4 => { directive: "L<", asm: "u32", ctype: "uint32_t" },
          8 => { directive: "Q<", asm: "u64", ctype: "uint64_t" }
        }
      },
      "float" => {
        name: "FloatNumType",
        sized: {
          4 => { directive: "e", asm: "f32", ctype: "float" },
          8 => { directive: "E", asm: "f64", ctype: "double" }
        }
      }
    }
    @@type_kinds["offset"] = @@type_kinds["int"]
    @@type_kinds["global"] = @@type_kinds["uint"]
    @@type_kinds["magic"] = @@type_kinds["uint"]
    
    def initialize bytes, kind, kind_name, pack_directive, asm, ctype
      @bytes = bytes
      @kind = kind
      @kind_name = kind_name
      @pack_directive = pack_directive
      @asm = asm
      @ctype = ctype
    end

    def to_s
      "#{@kind}#{@bytes * 8}"
    end

    def self.from_hash hash
      raise "Expect Hash for hash" unless hash.is_a? Hash
      t = hash["type"]
      b = hash["bytes"]
      raise "Expect String for type" unless t.is_a? String
      raise "Expect Integer for bytes" unless b.is_a? Integer

      tk = @@type_kinds[t]
      raise "Unknown type kind #{t}" if tk.nil?
      s = tk[:sized][b]
      raise "Invalid size #{b} bytes for #{tk[:name]}" if s.nil?

      new b, t, tk[:name], s[:directive], s[:asm], s[:ctype]
    end

    def check_val val
      case @kind_name
      when "IntNumType"
        # Check class
        raise "Expect Integer for IntNumType" unless val.is_a? Integer
        # Check range
        max = 2 ** (8 * @bytes - 1) - 1
        min = -2 ** (8 * @bytes - 1)
        raise "Value #{val} out of range #{min}..#{max}" unless min <= val && val <= max
      when "UintNumType"
        # Check class
        raise "Expect Integer for UintNumType" unless val.is_a? Integer
        # Check range
        max = 2 ** (8 * @bytes) - 1
        min = 0
        raise "Value #{val} out of range #{min}..#{max}" unless min <= val && val <= max
      when "FloatNumType"
        raise "Expect Float for FloatNumType" unless val.is_a? Float
      end
    end
  end

  # Type

  class Type
    def self.from_hash hash
      raise "Expect Hash for hash" unless hash.is_a? Hash
      t = hash["type"]
      b = hash["bytes"]
      raise "Expect String for type" unless t.is_a? String
      raise "Expect Integer for bytes" unless b.is_a? Integer
      if t == "array"
        elem = hash["elem"]
        raise "Expect Hash for elem" unless elem.is_a? Hash
        et = NumType.from_hash elem
        ArrayType.new b, et
      else
        LitType.new NumType.from_hash hash
      end
    end
  end

  class ArrayType < Type
    attr_reader :len_type, :elem_type

    def initialize bytes, elem_type
      @len_type = NumType.from_hash({"type" => "uint", "bytes" => bytes})
      raise "Expect NumType for elem_type" unless elem_type.is_a? NumType
      @elem_type = elem_type
    end
    
    def to_s
      "[array #{@len_type} => #{elem_type}]"
    end

    def pack array
      raise "Expect Array for array" unless array.is_a? Array
      len = array.length
      len_str = [len].pack @len_type.pack_directive
      elem_str = array.map do |x|
        [x].pack @elem_type.pack_directive
      end.join
      len_str + elem_str
    end

    def unpack str
      raise "Expect String for str" unless str.is_a? String

      # Unpack length
      directive = "#{@len_type.pack_directive}a*"
      len_a = str.unpack directive
      raise "Failed to unpack array length with a directive #{directive}" if len_a[0].nil?
      len = len_a[0]

      # Unpack elements
      directive = "#{@elem_type.pack_directive}#{len}a*"
      data_a = len_a[1].unpack directive
      raise "Failed to unpack data array with a directive #{directive}" if data_a[0].nil?
      
      return data_a[0...len], data_a[len]
    end
  end

  class LitType < Type
    attr_reader :type

    def initialize type
      raise "Expect NumType for type" unless type.is_a? NumType
      @type = type
    end

    def to_s
      "[#{@type}]"
    end

    def pack value
      [value].pack @type.pack_directive
    end

    def unpack str
      raise "Expect String for str" unless str.is_a? String
      # Unpack value
      directive = "#{@type.pack_directive}a*"
      arr = str.unpack directive
      raise "Failed to unpack value with format #{directive}" if arr[0].nil?
      return arr[0], arr[1]
    end
  end

  # --- Op Spec

  class Arg
    attr_reader :name, :type

    def initialize name, type
      raise "Expect String for name" unless name.is_a? String
      raise "Expect Type for type" unless type.is_a? Type
      @name = name.downcase
      @type = type
    end

    def self.from_hash types, hash
      raise "Expect Hash for types" unless types.is_a? Hash
      raise "Expect Hash for hash" unless hash.is_a? Hash
      name = hash["name"]
      type = types[hash["type"]]
      Arg.new name, type
    end

    def to_s
      "#{@name}:#{@type}"
    end
  end

  class Op
    attr_reader :index, :name, :args, :desc
    attr_accessor :aligned, :c_impl

    def initialize index, name, args, desc
      @index = index
      @name = name.downcase
      @args = args
      @desc = desc
    end

    def self.from_hash types, index, hash
      raise "Expect Hash for hash" unless hash.is_a? Hash
      name = hash["name"]
      args = hash["args"]
      desc = hash["desc"]
      raise "Expect String for name" unless name.is_a? String
      raise "Expect Array for args" unless args.is_a? Array
      raise "Expect String for desc" unless desc.is_a? String
      args = args.map do |arg|
        Arg.from_hash types, arg
      end
      op = Op.new index, name, args, desc
      op.c_impl = hash["c_impl"] if hash.has_key? "c_impl"
      op.aligned = hash["aligned"] if hash.has_key? "aligned"
      op
    end
    
    def to_s
      "Op[#{@index}:#{@name}](#{args.map{|x| x.to_s}.join(",")})"
    end
  end

  # --- Magic Spec

  class Magic
    attr_reader :index, :name, :desc
    attr_accessor :c_impl

    def initialize index, name, desc
      @index = index
      @name = name.downcase
      @desc = desc
    end

    def self.from_hash index, hash
      raise "Expect Integer for index" unless index.is_a? Integer
      raise "Expect Hash for hash" unless hash.is_a? Hash
      name = hash["name"]
      desc = hash["desc"]
      raise "Expect String for name" unless name.is_a? String
      raise "Expect String for desc" unless desc.is_a? String
      mg = Magic.new index, name, desc
      mg.c_impl = hash["c_impl"] if hash.has_key? "c_impl"
      mg
    end
    
    def to_s
      "Magic[#{@index}:#{@name}]"
    end
  end

  # --- Spec structure

  class Spec
    attr_reader :types, :ops, :ops_arr, :magics, :magic_arr

    def initialize
      @types = {}
      @ops = {}
      @ops_arr = []
      @magics = {}
      @magic_arr = []
    end

    def load_from_hash hash
      raise "Expect Hash for hash" unless hash.is_a? Hash

      # Load args
      @types = {}
      hash["types"].each do |k, v|
        @types[k.downcase] = Type.from_hash v
      end

      # Load ops
      @ops = {}
      @ops_arr = []
      hash["opcodes"].each do |op|
        idx = @ops_arr.length
        op = Op.from_hash @types, idx, op
        @ops_arr << op
        @ops[op.name] = op
      end

      @magics = {}
      @magic_arr = []
      hash["magics"].each do |magic|
        idx = @magic_arr.length
        magic = Magic.from_hash idx, magic
        @magic_arr << magic
        @magics[magic.name] = magic
      end

      self
    end

    def type key
      @types[key.downcase]
    end

    def op key
      if key.is_a? Integer
        @ops_arr[key]
      else
        @ops[key.downcase]
      end
    end

    def magic key
      if key.is_a? Integer
        @magic_arr[key]
      else
        @magics[key.downcase]
      end
    end

    def load_from_json json
      load_from_hash JSON.parse json
    end

    def load_from_json_file filename
      load_from_json File.read filename
    end
  end

  def self.spec
    json_filename = ENV["SSM_SPEC_JSON"]
    if json_filename.nil?
      script_path = File.expand_path __FILE__
      json_filename = File.join File.dirname(script_path), "spec.json"
    end
    Spec.new.load_from_json_file json_filename
  end
end