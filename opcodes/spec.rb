# spec.rb: SSM Spec Helper Classes
# Author: lumiknit

require 'json'

module SSM

  # --- Argument Spec

  # Raw Type (Int, Uint, Float)
  class NumType
    attr_reader :bytes, :kind, :pack_directive, :asm, :ctype
    
    def initialize bytes, kind, pack_directive, asm, ctype
      @bytes = bytes
      @kind = kind
      @pack_directive = pack_directive
      @asm = asm
      @ctype = ctype
    end

    def to_s
      "#{@kind}#{@bytes}"
    end

    def self.from_hash hash
      raise "Expect Hash for hash" unless hash.is_a? Hash
      t = hash["type"]
      b = hash["bytes"]
      raise "Expect String for type" unless t.is_a? String
      raise "Expect Integer for bytes" unless b.is_a? Integer
      case t
      when "int", "offset"
        directives = {1 => "c", 2 => "s<", 4 => "l<", 8 => "q<"}
        pack = directives[b]
        raise "Invalid size for IntRawType" unless pack
        new b, t, pack, "i#{8 * b}", "int#{8 * b}_t"
      when "uint", "magic"
        directives = {1 => "C", 2 => "S<", 4 => "L<", 8 => "Q<"}
        pack = directives[b]
        raise "Invalid size for IntRawType" unless pack
        new b, t, pack, "u#{8 * b}", "uint#{8 * b}_t"
      when "float"
        directives = {4 => "e", 8 => "E"}
        pack = directives[b]
        raise "Invalid size for IntRawType" unless pack
        new b, t, pack, "f#{8 * b}", "float#{8 * b}_t"
      else
        raise "Unknown type #{t} and bytes #{b}"
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
      len_a = str.unpack "{@len_type.pack_directive}a"
      raise "Failed to unpack array length" if len_a[0].is_nil?
      len = len_a[0]
      data_a = len_a[1].unpack "#{@elem_type.pack_directive}#{len}"
      raise "Failed to unpack data array" if data_a[0].is_nil?
      return data_a[0], data_a[1]
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
      directive = "#{@type.pack_directive}a"
      arr = str.unpack directive
      raise "Failed to unpack value" if arr[0].is_nil?
      return arr[0], arr[1]
    end
  end

  # --- Op Spec

  class Arg
    attr_reader :name, :type

    def initialize name, type
      raise "Expect String for name" unless name.is_a? String
      raise "Expect Type for type" unless type.is_a? Type
      @name = name
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

    def initialize index, name, args, desc
      @index = index
      @name = name
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
      Op.new index, name, args, desc
    end
    
    def to_s
      "Op[#{@index}:#{@name}](#{args.map{|x| x.to_s}.join(",")})"
    end
  end

  # --- Magic Spec

  class Magic
    attr_reader :index, :name, :desc

    def initialize index, name, desc
      @index = index
      @name = name
      @desc = desc
    end

    def self.from_hash index, hash
      raise "Expect Integer for index" unless index.is_a? Integer
      raise "Expect Hash for hash" unless hash.is_a? Hash
      name = hash["name"]
      desc = hash["desc"]
      raise "Expect String for name" unless name.is_a? String
      raise "Expect String for desc" unless desc.is_a? String
      Magic.new index, name, desc
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
        @types[k] = Type.from_hash v
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

    def load_from_json json
      load_from_hash JSON.parse json
    end

    def load_from_json_file filename
      load_from_json File.read filename
    end
  end

  def spec
    json_filename = ENV["SSM_SPEC_JSON"]
    json_filename = "spec.json" if json_filename.is_nil?
    @Spec.new.load_from_json_file json_filename
  end
end