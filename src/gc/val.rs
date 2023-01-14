// val.rs: ssm value

use std::fmt;

// -- Bit-trick value

// Pointer-size float

// 64-bit
#[cfg(target_pointer_width = "64")]
pub type Fptr = f64;
#[cfg(target_pointer_width = "64")]
pub type Iptr = i64;
#[cfg(target_pointer_width = "64")]
pub type Uptr = u64;

// 32-bit
#[cfg(target_pointer_width = "32")]
pub type Fptr = f32;
#[cfg(target_pointer_width = "32")]
pub type Iptr = i32;
#[cfg(target_pointer_width = "32")]
pub type Uptr = u32;

// Val type

#[derive(PartialEq, Debug, Clone, Copy)]
pub struct Val(pub Iptr);

impl fmt::Display for Val {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "Val({:x})", self.0)
    }
}

impl Val {
    // to_type
    #[inline(always)]
    pub fn to_usize(self) -> usize {
        self.0 as usize
    }

    #[inline(always)]
    pub fn to_uint(self) -> Uptr {
        (self.0 as Uptr) >> 1
    }

    #[inline(always)]
    pub fn to_int(self) -> Iptr {
        self.0 >> 1
    }

    #[inline(always)]
    pub fn to_flt(self) -> Fptr {
        Fptr::from_bits((self.0 as Uptr) & !(1 as Uptr))
    }

    #[inline(always)]
    pub fn to_ptr<T>(self) -> *mut T {
        (self.0 - (1 as Iptr)) as *mut T
    }

    #[inline(always)]
    pub fn to_gc_ptr<T>(self) -> *mut T {
        self.0 as *mut T
    }

    // from_type
    #[inline(always)]
    pub fn from_usize(val: usize) -> Self {
        Val(val as Iptr)
    }

    #[inline(always)]
    pub fn from_uint(val: Uptr) -> Self {
        Val(((val as Iptr) << 1) | (1 as Iptr))
    }

    #[inline(always)]
    pub fn from_int(val: Iptr) -> Self {
        Val(((val as Iptr) << 1) | (1 as Iptr))
    }

    #[inline(always)]
    pub fn from_flt(val: Fptr) -> Self {
        Val((val.to_bits() as Iptr) | (1 as Iptr))
    }

    #[inline(always)]
    pub fn from_ptr<T>(ptr: *const T) -> Self {
        Val((ptr as Iptr) + (1 as Iptr))
    }

    #[inline(always)]
    pub fn from_gc_ptr<T>(ptr: *const T) -> Self {
        Val(ptr as Iptr)
    }

    // check type
    #[inline(always)]
    pub fn is_ptr(self) -> bool {
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
 * | <- high                               low -> |
 *   * Small object (Tagged tuple)
 * | color (2b) | 0 (1b) | size (13b) | tag (16b) |
 *   * Large object (Raw bytes)
 * | color (2b) | 1 (1b) |       size  (29b)      |
 *
 * [64-bit]
 * | <- high                               low -> |
 *   * Small object (Tagged tuple)
 * | color (2b) | 0 (1b) | size (45b) | tag (16b) |
 *   * Large object (Raw bytes)
 * | color (2b) | 1 (1b) |       size  (61b)      |
 */

#[derive(PartialEq, Debug, Clone, Copy)]
pub struct Hd(pub Uptr);

#[allow(dead_code)]
impl Hd {
    pub const COLOR_SHIFT: u32 = Uptr::BITS - 2;
    pub const COLOR_MASK: Uptr = (0x3 as Uptr) << Self::COLOR_SHIFT;
    pub const COLOR_INV_MASK: Uptr = !Self::COLOR_MASK;
    pub const COLOR_H: Uptr = (0x2 as Uptr) << Self::COLOR_SHIFT;
    pub const COLOR_L: Uptr = (0x1 as Uptr) << Self::COLOR_SHIFT;

    pub const COLOR_BLACK: Uptr = Self::COLOR_L | Self::COLOR_H;
    pub const COLOR_WHITE: Uptr = 0;
    pub const COLOR_GRAY: Uptr = Self::COLOR_L;

    pub const LONG_BIT_SHIFT: u32 = Uptr::BITS - 3;
    pub const LONG_BIT: Uptr = (0x1 as Uptr) << Self::LONG_BIT_SHIFT;

    pub const LONG_SIZE_MASK: Uptr = !(Self::COLOR_MASK | Self::LONG_BIT);

    pub const TAG_MASK: Uptr = 0xffff;

    pub const SIZE_SHIFT: u32 = 16;
    pub const SIZE_MASK: Uptr =
        !(Self::COLOR_MASK | Self::LONG_BIT | Self::TAG_MASK);

    #[inline(always)]
    pub fn from_val(val: Val) -> Self {
        Self(val.0 as Uptr)
    }

    #[inline(always)]
    pub fn to_val(&self) -> Val {
        Val(self.0 as Iptr)
    }
    
    #[inline(always)]
    pub fn color(&self) -> Uptr {
        self.0 & Self::COLOR_MASK
    }

    #[inline(always)]
    pub fn is_black(&self) -> bool {
        Self::COLOR_BLACK == (self.0 & Self::COLOR_MASK)
    }

    #[inline(always)]
    pub fn is_gray(&self) -> bool {
        Self::COLOR_GRAY == (self.0 & Self::COLOR_MASK)
    }

    #[inline(always)]
    pub fn is_white(&self) -> bool {
        Self::COLOR_WHITE == (self.0 & Self::COLOR_MASK)
    }

    #[inline(always)]
    pub fn mark(&mut self, color: Uptr) {
        *self = Self(self.0 | (color & Self::COLOR_MASK));
    }

    #[inline(always)]
    pub fn unmark(&mut self) {
        *self = Self(self.0 & Self::COLOR_INV_MASK);
    }

    #[inline(always)]
    pub fn is_long(&self) -> bool {
        0 != (self.0 & Self::LONG_BIT)
    }

    #[inline(always)]
    pub fn tag(&self) -> Uptr {
        self.0 & Self::TAG_MASK
    }

    #[inline(always)]
    pub fn long_size(&self) -> Uptr {
        self.0 & Self::LONG_SIZE_MASK
    }

    #[inline(always)]
    pub fn long_bytes(&self) -> Uptr {
        (self.0 & Self::LONG_SIZE_MASK) * ((Uptr::BITS / 8) as Uptr)
    }

    #[inline(always)]
    pub fn size(&self) -> Uptr {
        (self.0 & Self::SIZE_MASK) >> Self::SIZE_SHIFT
    }

    #[inline(always)]
    pub fn short(size: Uptr, tag: Uptr) -> Self {
        Self(
            Self::COLOR_BLACK
                | ((size << Self::SIZE_SHIFT) & Self::SIZE_MASK)
                | (tag & Self::TAG_MASK),
        )
    }

    #[inline(always)]
    pub fn long(size: Uptr) -> Self {
        Self(Self::COLOR_BLACK | Self::LONG_BIT | (size & Self::LONG_SIZE_MASK))
    }
}

#[derive(PartialEq, Debug, Clone, Copy)]
pub struct Tup(pub *mut Uptr);

impl Tup {
    pub fn header(self) -> Hd {
        unsafe {
            Hd(self.0.read())
        }
    }

    pub fn set_header(self, hd: Hd) {
        unsafe {
            self.0.write(hd.0);
        }
    }

    pub fn val(self, idx: usize) -> Val {
        unsafe {
            Val(self.0.add(idx + 1).read() as Iptr)
        }
    }

    pub fn set_val(self, idx: usize, val: Val) {
        unsafe {
            self.0.add(idx + 1).write(val.0 as Uptr);
        }
    }

    pub fn bytes(self) -> *mut u8 {
        unsafe {
            self.0.add(1) as *mut u8
        }
    }

    pub fn byte_at(self, idx: usize) -> u8 {
        unsafe {
            self.bytes().add(idx).read()
        }
    }
}
