// val.rs: ssm value

use std::fmt;

// -- Bit-trick value

pub const WORD_SIZE: usize = std::mem::size_of::<usize>();
pub const WORD_SHIFT: u32 = WORD_SIZE.trailing_zeros();

// Pointer-size float

// 64-bit
#[cfg(target_pointer_width = "64")]
pub type Fsize = f64;

// 32-bit
#[cfg(target_pointer_width = "32")]
pub type Fsize = f32;

// Val type

#[derive(PartialEq, Debug, Clone, Copy)]
pub struct Val(pub usize);

impl fmt::Display for Val {
  fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
    write!(f, "Val({:x})", self.0)
  }
}

impl Val {
  // Type conversions

  // bytes: as raw bytes
  #[inline(always)]
  pub fn to_bytes(self) -> [u8; std::mem::size_of::<usize>()] {
    self.0.to_le_bytes()
  }

  #[inline(always)]
  pub fn from_bytes(bytes: [u8; std::mem::size_of::<usize>()]) -> Self {
    Self(usize::from_le_bytes(bytes))
  }

  // isize: as raw bytes
  #[inline(always)]
  pub fn to_isize(self) -> isize {
    isize::from_ne_bytes(self.to_bytes())
  }

  #[inline(always)]
  pub fn from_isize(x: isize) -> Self {
    Self::from_bytes(x.to_ne_bytes())
  }

  // uint: packaged unsinged integer
  #[inline(always)]
  pub fn to_uint(self) -> usize {
    self.0 >> 1
  }

  #[inline(always)]
  pub fn from_uint(x: usize) -> Self {
    Val((x << 1) | (1 as usize))
  }

  // int: packaged signed integer
  #[inline(always)]
  pub fn to_int(self) -> isize {
    self.to_isize() >> 1
  }

  #[inline(always)]
  pub fn from_int(x: isize) -> Self {
    Self::from_isize((x << 1) | (1 as isize))
  }

  // flt: packaged float
  #[inline(always)]
  pub fn to_flt(self) -> Fsize {
    Fsize::from_ne_bytes((self.0 & !(1 as usize)).to_ne_bytes())
  }

  #[inline(always)]
  pub fn from_flt(x: Fsize) -> Self {
    Val(usize::from_ne_bytes(x.to_ne_bytes()) | (1 as usize))
  }

  // raw_ptr: pointer which is not managed by GC (literal)
  // must aligned by 2
  #[inline(always)]
  pub fn to_raw_ptr<T>(self) -> *mut T {
    (self.0 & (!(1 as usize))) as *mut T
  }

  #[inline(always)]
  pub fn from_raw_ptr<T>(ptr: *mut T) -> Self {
    Val(ptr as usize | (1 as usize))
  }

  // gc_ptr: pointer which is managed by GC
  // must aligned by 2
  #[inline(always)]
  pub fn to_gc_ptr<T>(self) -> *mut T {
    self.0 as *mut T
  }

  #[inline(always)]
  pub fn from_gc_ptr<T>(ptr: *mut T) -> Self {
    Val(ptr as usize)
  }

  // check type
  #[inline(always)]
  pub fn is_gc_ptr(self) -> bool {
    (self.0 & 1) == 0
  }

  #[inline(always)]
  pub fn is_lit(self) -> bool {
    (self.0 & 1) != 0
  }
}

// -- GC Header

/* Header Representation
 * [32-bit]
 * | <- high                 low -> |
 *   * Small object (Tagged tuple)
 * | color (2b) | 0 (1b) | size (13b) | tag (16b) |
 *   * Large object (Raw bytes)
 * | color (2b) | 1 (1b) |     size  (29b)    |
 *
 * [64-bit]
 * | <- high                 low -> |
 *   * Small object (Tagged tuple)
 * | color (2b) | 0 (1b) | size (45b) | tag (16b) |
 *   * Large object (Raw bytes)
 * | color (2b) | 1 (1b) |     size  (61b)    |
 */

#[derive(PartialEq, Debug, Clone, Copy)]
pub struct Hd(pub usize);

#[allow(dead_code)]
impl Hd {
  pub const COLOR_SHIFT: u32 = usize::BITS - 2;
  pub const COLOR_H: usize = (0x1 as usize) << (1 + Self::COLOR_SHIFT);
  pub const COLOR_L: usize = (0x1 as usize) << Self::COLOR_SHIFT;
  pub const COLOR_MASK: usize = Self::COLOR_H | Self::COLOR_L;
  pub const COLOR_INV_MASK: usize = !Self::COLOR_MASK;

  pub const COLOR_BLACK: usize = Self::COLOR_L | Self::COLOR_H;
  pub const COLOR_WHITE: usize = 0;
  pub const COLOR_GRAY: usize = Self::COLOR_L;
  pub const COLOR_RED: usize = Self::COLOR_H;

  pub const LONG_BIT_SHIFT: u32 = usize::BITS - 3;
  pub const LONG_BIT: usize = (0x1 as usize) << Self::LONG_BIT_SHIFT;

  pub const LONG_SIZE_MASK: usize = !(Self::COLOR_MASK | Self::LONG_BIT);

  pub const TAG_MASK: usize = 0xffff;

  pub const SIZE_SHIFT: u32 = 16;
  pub const SIZE_MASK: usize =
    !(Self::COLOR_MASK | Self::LONG_BIT | Self::TAG_MASK);

  // Conversion between val
  #[inline(always)]
  pub fn from_val(val: Val) -> Self {
    Self(val.0)
  }

  #[inline(always)]
  pub fn to_val(&self) -> Val {
    Val(self.0)
  }

  #[inline(always)]
  pub fn color(self) -> usize {
    self.0 & Self::COLOR_MASK
  }

  #[inline(always)]
  pub fn is_black(self) -> bool {
    Self::COLOR_BLACK == self.color()
  }

  #[inline(always)]
  pub fn is_gray(self) -> bool {
    Self::COLOR_GRAY == self.color()
  }

  #[inline(always)]
  pub fn is_white(self) -> bool {
    Self::COLOR_WHITE == self.color()
  }

  #[inline(always)]
  pub fn marked(self, color: usize) -> Self {
    Self(self.0 | (color & Self::COLOR_MASK))
  }

  #[inline(always)]
  pub fn unmarked(&self) -> Self {
    Self(self.0 & Self::COLOR_INV_MASK)
  }

  #[inline(always)]
  pub fn is_long(self) -> bool {
    0 != (self.0 & Self::LONG_BIT)
  }

  #[inline(always)]
  pub fn tag(self) -> u16 {
    (self.0 & Self::TAG_MASK) as u16
  }

  #[inline(always)]
  pub fn long_bytes(self) -> usize {
    self.0 & Self::LONG_SIZE_MASK
  }

  #[inline(always)]
  pub fn long_words(self) -> usize {
    ((self.0 & Self::LONG_SIZE_MASK) + WORD_SIZE - 1) / WORD_SIZE
  }

  #[inline(always)]
  pub fn short_words(self) -> usize {
    (self.0 & Self::SIZE_MASK) >> Self::SIZE_SHIFT
  }

  #[inline(always)]
  pub fn bytes(self) -> usize {
    if self.is_long() {
      self.long_bytes()
    } else {
      self.short_words() * WORD_SIZE
    }
  }

  #[inline(always)]
  pub fn words(self) -> usize {
    if self.is_long() {
      self.long_words()
    } else {
      self.short_words()
    }
  }

  #[inline(always)]
  pub fn len(self) -> usize {
    if self.is_long() {
      self.long_bytes()
    } else {
      self.short_words()
    }
  }

  #[inline(always)]
  pub fn short(size: usize, tag: u16) -> Self {
    Self(
      Self::COLOR_WHITE
        | ((size << Self::SIZE_SHIFT) & Self::SIZE_MASK)
        | ((tag as usize) & Self::TAG_MASK),
    )
  }

  #[inline(always)]
  pub fn long(size: usize) -> Self {
    Self(Self::COLOR_WHITE | Self::LONG_BIT | (size & Self::LONG_SIZE_MASK))
  }
}

#[derive(PartialEq, Debug, Clone, Copy)]
pub struct Tup(pub *mut usize);

impl Tup {
  pub const MINOR_EXTRA_WORDS: usize = 1;
  pub const MAJOR_EXTRA_WORDS: usize = Self::MINOR_EXTRA_WORDS + 1;

  pub const NULL: Self = Self(std::ptr::null_mut());

  // Null
  #[inline(always)]
  pub fn is_null(self) -> bool {
    self.0.is_null()
  }

  // Type conversion
  #[inline(always)]
  pub fn from_val(val: Val) -> Self {
    Self(val.to_gc_ptr())
  }

  #[inline(always)]
  pub fn to_val(self) -> Val {
    Val::from_gc_ptr(self.0)
  }

  // Size (in words) calculation
  #[inline(always)]
  pub fn words_from_words(words: usize) -> usize {
    words + 1
  }

  #[inline(always)]
  pub fn words_from_bytes(bytes: usize) -> usize {
    1 + (bytes + WORD_SIZE - 1) / WORD_SIZE
  }

  // Header
  #[inline(always)]
  pub fn header(self) -> Hd {
    unsafe { Hd(self.0.read()) }
  }

  #[inline(always)]
  pub fn set_header(self, hd: Hd) {
    unsafe { self.0.write(hd.0) }
  }

  #[inline(always)]
  pub fn gc_next_ptr<T>(self) -> *mut T {
    unsafe { self.0.sub(1) as *mut T }
  }

  #[inline(always)]
  pub fn gc_next(self) -> Self {
    unsafe {
      let gc_next_ptr = self.gc_next_ptr::<*mut usize>();
      Self(gc_next_ptr.read())
    }
  }

  #[inline(always)]
  pub fn set_gc_next(self, ptr: Tup) {
    unsafe {
      let gc_next_ptr = self.gc_next_ptr::<*mut usize>();
      gc_next_ptr.write(ptr.0)
    }
  }

  // Major object linked list
  #[inline(always)]
  pub fn major_next_ptr<T>(self) -> *mut T {
    unsafe { self.0.sub(2) as *mut T }
  }

  #[inline(always)]
  pub fn major_next(self) -> Self {
    unsafe {
      let next_ptr = self.major_next_ptr::<*mut usize>();
      Self(next_ptr.read())
    }
  }

  #[inline(always)]
  pub fn set_major_next(self, ptr: Tup) {
    unsafe {
      let next_ptr = self.major_next_ptr::<*mut usize>();
      next_ptr.write(ptr.0)
    }
  }

  // Values

  #[inline(always)]
  pub fn elem(self, idx: usize) -> Val {
    unsafe { Val(self.0.add((idx as usize) + 1).read()) }
  }

  #[inline(always)]
  pub fn set_elem(self, idx: usize, val: Val) {
    unsafe {
      self.0.add((idx as usize) + 1).write(val.0 as usize);
    }
  }

  #[inline(always)]
  pub fn bytes(self) -> *mut u8 {
    unsafe { self.0.add(1) as *mut u8 }
  }

  #[inline(always)]
  pub fn byte_at(self, idx: usize) -> u8 {
    unsafe { self.bytes().add(idx as usize).read() }
  }

  #[inline(always)]
  pub fn set_byte_at(self, idx: usize, val: u8) {
    unsafe {
      self.bytes().add(idx as usize).write(val);
    }
  }
}