pico-8 cartridge // http://www.pico-8.com
version 16
__lua__

--
-- main entry point for unz8()
--
function unz8(data_string, data_address, data_length)
  -- [minify] replaces: data_string s char_lut t
  -- [minify] replaces: data_address y state z data_length x bit_buffer w temp_buffer v available_bits u

  -- init stream reader
  local bit_buffer = 0      -- bit buffer, starting from bit 0 (= 0x.0001)
  local available_bits = 0  -- number of bits in buffer
  local temp_buffer = 0     -- temp chunk buffer
  local state = 0           -- 0: nothing in accumulator, 1: 2 chunks remaining, 2: 1 chunk remaining

  -- [minify] replaces: flush_bits f peek_bits g

  -- get rid of n first bits
  local function flush_bits(nbits)
    -- [minify] replaces: nbits i
    available_bits -= nbits
    bit_buffer = lshr(bit_buffer, nbits)
  end

  -- cast a value to a number (trick: can use lshr() instead!)
  -- [minify] replaces: cast_to_num lshr
  local function cast_to_num(x) -- debug
    return lshr(x)              -- debug
  end                           -- debug

  -- lookup table for peek_bits()
  --  - indices 1 and 2 are the higher bits (>=32) of 59^7 and 59^8
  --    used to compute these powers
  --  - string indices are for char -> byte lookups; the order in the
  --    base string is not important but we exploit it to make our
  --    compressed code shorter
  local char_lut = { 9, 579 }
  for i = 1, 58 do char_lut[sub(",i])v+=e%1*c579}f#k<lmax>0q/42368ghjnprwyz!{:;.~_do t[sub(", i, i)] = i end

  -- peek n bits from the stream
  local function peek_bits(nbits)
    -- [minify] replaces: nbits i
    while available_bits < nbits do
      -- not enough data in the bit buffer:
      -- if there is still data in memory, read the next byte; otherwise
      -- unpack the next 8 characters of base59 data into 47 bits of
      -- information that we insert into bit_buffer in chunks of 16 or
      -- 15 bits.
      if data_length and data_length > 0 then
        bit_buffer += lshr(peek(data_address), 16 - available_bits)
        available_bits += 8
        data_address += 1
        data_length -= 1
      elseif state < 1 then
        temp_buffer = 0
        local p = 0
        local e = 2^-16
        for i = 1, 8 do
          local c = cast_to_num(char_lut[sub(data_string, i, i)])
          temp_buffer += e % 1 * c
          p += (lshr(e, 16) + cast_to_num(char_lut[i - 6])) * c
          e *= 59
        end
        data_string = sub(data_string, 9)
        bit_buffer += shl(temp_buffer % 1, available_bits)
        available_bits += 16
        state += 1
        temp_buffer = lshr(temp_buffer, 16) + p
      elseif state < 2 then
        bit_buffer += shl(temp_buffer % 1, available_bits)
        available_bits += 16
        state += 1
        temp_buffer = lshr(temp_buffer, 16)
      else
        bit_buffer += shl(temp_buffer % 1, available_bits)
        available_bits += 15
        state = 0
      end
    end
    --printh("peek_bits("..nbits..") = "..strx(lshr(shl(bit_buffer, 32-nbits), 16-nbits))
    --       .." [bit_buffer = "..strx(shl(bit_buffer, 16)).."]")
    return (lshr(shl(bit_buffer, 32 - nbits), 16 - nbits))
    -- this cannot work because of read_bits(16)
    -- maybe bring this back if we disable uncompressed blocks?
    -- or maybe only allow 15-bit-length uncompressed blocks?
    --return band(shl(bit_buffer, 16), 2 ^ nbits - 1)
  end

  -- [minify] can reuse: data_string s char_lut t
  -- [minify] can reuse: data_address y data_length x bit_buffer w temp_buffer v available_bits u state z
  -- [minify] replaces: read_bits u read_symbol v

  -- get a number of n bits from stream and flush them
  local function read_bits(nbits)
    -- [minify] replaces: nbits i
    return peek_bits(nbits), flush_bits(nbits)
  end

  -- get next variable value from stream, according to huffman table
  local function read_symbol(huff_tree)
    -- [minify] replaces: huff_tree i
    -- require at least n bits, even if only p<n bytes may be actually consumed
    local j = peek_bits(huff_tree.max_bits)
    flush_bits(huff_tree[j] % 1 * 16)
    return (flr(huff_tree[j]))
  end

  -- [minify] can reuse: peek_bits g flush_bits f
  -- [minify] replaces: build_huff_tree g

  -- build a huffman table
  local function build_huff_tree(huff_tree_desc)
    -- [minify] replaces: huff_tree_desc i max_bits j tree t reversed_code z code u
    local tree = { max_bits = 1 }
    for j = 1, 288 do
      tree.max_bits = max(tree.max_bits, cast_to_num(huff_tree_desc[j]))
    end
    local code = 0
    for l = 1, 18 do -- for some reason "18" compresses better than "17" or even "16"!
      for j = 1, 288 do
        if l == huff_tree_desc[j] then
          -- flip the first l bits of the current code
          local reversed_code = 0
          for j = 1, l do reversed_code += shl(band(lshr(code, j - 1), 1), l - j) end
          -- store all possible n-bit values that end with flip(code)
          while reversed_code < 2 ^ tree.max_bits do
            tree[reversed_code] = j - 1 + l / 16
            reversed_code += 2 ^ l
          end
          code += 1
        end
      end
      code += code
    end
    return (tree) -- "return t end" has as many tokens as "return(t)end" but has lower entropy
  end

  -- [minify] replaces: write_byte f
  -- [minify] replaces: output_buffer t output_pos w

  -- init stream writer
  local output_buffer = {} -- output array (32-bit numbers)
  local output_pos = 1     -- output position, only used in write_byte() and do_block()

  -- write_byte 8 bits to the output, packed into a 32-bit number
  local function write_byte(byte)
    -- [minify] replaces: byte i
    local j = (output_pos) % 1  -- the parentheses here help compressing the code!
    local k = flr(output_pos)
    output_buffer[k] = rotl(byte, j * 32 - 16) + cast_to_num(output_buffer[k])
    output_pos += 1 / 4
  end

  --
  -- main loop
  --
  for j = 1, 288 do -- minifying trick; there's never going to be 288 blocks!
    if read_bits(1) < 1 then
      if read_bits(1) < 1 then
        return (output_buffer)
      end
      -- inflate uncompressed byte array
      -- we do not align the input buffer to a byte boundary, because there
      -- is no concept of byte boundary in a stream we read in 47-bit chunks.
      -- also, we do not store the bit complement of the length value, it is
      -- not really important with such small data.
      for i = 1, read_bits(16) do
        write_byte(read_bits(8))
      end
    else
      -- [minify] replaces: lit_tree_desc k len_tree_desc q
      -- [minify] replaces: lit_count l len_count i tree_desc t
      local lit_tree_desc = {}
      local len_tree_desc = {}
      if read_bits(1) < 1 then
        -- inflate static block
        for j =   1, 288 do lit_tree_desc[j] = 8 end
        for j = 145, 280 do lit_tree_desc[j] += sgn(256 - j) end
        for j =   1,  32 do len_tree_desc[j] = 5 end
      else
        -- inflate dynamic block
        local lit_count = 257 + read_bits(5)
        local len_count = 1 + read_bits(5)
        local tree_desc = {}
        -- the formula below differs from official deflate
        --  deflate: {17,18,19,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16}
        --  j%19+1:  {17,18,19,1,9,8,10,7,11,6,12,5,13,4,14,3,15,2,16}
        for j = -3, read_bits(4) do tree_desc[j % 19 + 1] = read_bits(3) end
        local g = build_huff_tree(tree_desc)

        -- [minify] replaces: read_tree_desc r description k count l
        local function read_tree_desc(description, count)
          while #description < count do
            local g = read_symbol(g)
            if g >= 19 then                                                        -- debug
              error("wrong entry in depth table for literal/length alphabet: "..g) -- debug
            end                                                                    -- debug
                if g == 16 then for j = -2, read_bits(2)     do add(description, description[#description]) end
            elseif g == 17 then for j = -2, read_bits(3)     do add(description, 0) end
            elseif g == 18 then for j = -2, read_bits(7) + 8 do add(description, 0) end
            else add(description, g) end
          end
        end

        read_tree_desc(lit_tree_desc, lit_count)
        read_tree_desc(len_tree_desc, len_count)
      end

      lit_tree_desc = build_huff_tree(lit_tree_desc)
      len_tree_desc = build_huff_tree(len_tree_desc)

      -- [minify] replaces: read_varint g sym_code i
      local function read_varint(sym_code, j)
        if sym_code > j then
          local k = flr(sym_code / j - 1)
          sym_code = shl(sym_code % j + j, k) + read_bits(k)
        end
        return (sym_code)
      end

      -- decompress the block using the two huffman tables
      -- [minify] replaces: symbol i size_minus_3 l distance q
      local symbol = read_symbol(lit_tree_desc)
      while symbol != 256 do
        if symbol < 256 then
          -- write a literal symbol to the output
          write_byte(symbol)
        else
          local size_minus_3 = symbol < 285 and read_varint(symbol - 257, 4) or 255
          local distance = 1 + read_varint(read_symbol(len_tree_desc), 2)
          -- read back all bytes and append them to the output
          for j = -2, size_minus_3 do
            local j = (output_pos - distance / 4) % 1
            local k = flr(output_pos - distance / 4)
            write_byte(band(rotr(output_buffer[k], j * 32 - 16), 255))
          end
        end
        symbol = read_symbol(lit_tree_desc)
      end
    end
  end
end

--
-- debug function to display hex numbers with minimal chars
--
local function strx(nbits)                                  -- debug
  local s = sub(tostr(nbits, 1), 3, 6)                      -- debug
  while #s > 1 and sub(s, 1, 1) == "0" do s = sub(s, 2) end -- debug
  return "0x"..s                                            -- debug
end                                                         -- debug

--
-- error reporting
--
local function error(s) -- debug
  printh(s)             -- debug
  abort()               -- debug
end                     -- debug

--
-- print to stdout using ^ and M- notation
--
local function puts(t)                                                     -- debug
  local function chr(i) return sub(" !\"#$%&'()*+,-./0123456789:;<=>?@"..  -- debug
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\127", -- debug
    i - 32, i - 32)                                                        -- debug
  end                                                                      -- debug
  local lut = {}                                                           -- debug
  for i = 1, 128 do lut[i] = "^"..chr(bxor(i, 64)) end                     -- debug
  for i = 32, 127 do lut[i] = chr(i) end                                   -- debug
  for i = 129, 256 do lut[i] = "M-"..lut[i - 128] end                      -- debug
  lut[11] = "\n" lut[14] = "\r"                                            -- debug
  local s = ""                                                             -- debug
  for i = 1, #t do                                                         -- debug
    for j = 2, 5 do                                                        -- debug
      s = s..lut[1 + band(rotr(t[i], 8 * j), 255)]                         -- debug
    end                                                                    -- debug
  end                                                                      -- debug
  printh(s)                                                                -- debug
end                                                                        -- debug

