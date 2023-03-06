module Arg where

import qualified Data.ByteString as B
import qualified Data.Binary.Get as BG
import qualified Magic

data ArgType =
    TInt Int -- Signed int (bits)
  | TUint Int -- Unsigned int (bits)
  | TFloat Int -- Float (bits)
  | TBytes Int -- Bytes (bits of length)
  | TMagic Int -- Magic (bits of instr)
  | TOffset Int -- Offset (bits of offset)
  | TJmptbl Int -- Jump table (bits of offset)
deriving (Eq)

instance Show ArgType where
  show (TInt bits) = "i" ++ show bits
  show (TUint bits) = "u" ++ show bits
  show (TFloat bits) = "f" ++ show bits
  show (TBytes bits) = "b" ++ show bits
  show (TMagic bits) = "m" ++ show bits
  show (TOffset bits) = "o" ++ show bits
  show (TJmptbl bits) = "j" ++ show bits

i8 = AInt 8
i16 = AInt 16
i32 = AInt 32
u8 = AUint 8
u16 = AUint 16
u32 = AUint 32
f32 = AFloat 32
b16 = ABytes 16
b32 = ABytes 32
m16 = AMagic 16
o32 = AOffset 32
j32 = AJmptbl 32

data ArgVal =
    VInt Int32
  | VUint Word32
  | VFloat Float
  | VBytes ByteString
  | VMagic Int
  | VOffset Int32
  | VJmptbl [Int32]
deriving (Eq)

instance Show ArgVal where
  show (VInt i) = show i
  show (VUint i) = show i
  show (VFloat f) = show f
  show (VBytes bs) = show $ B.toString bs
  show (VMagifc i) = Magic.magics !! i
  show (VOffset i) = show i 
  show (VJmptbl is) = show is

unpackBytes :: Arg -> ByteString -> Either String (Val, ByteString)

unpackBytes (AInt 8) bs =
  let (val, rest) = B.splitAt 1 bs
  let (val, rest) = B.splitAt (bits `div` 8) bs
  return (VInt (B.foldl' (\a b -> a * 256 + fromIntegral b) 0 val), rest)

unpackBytes (AInt bits) bs = Left $ "Invalid int bits " ++ show bits
unpackBytes (AUint bits) bs = Left $ "Invalid uint bits" ++ show bits
unpackBytes (AFloat bits) bs = Left $ "Invalid float bits " ++ show bits
unpackBytes (ABytes bits) bs = Left $ "Invalid bytes bits " ++ show bits
unpackBytes (AMagic bits) bs = Left $ "Invalid magic bits " ++ show bits
unpackBytes (AOffset bits) bs = Left $ "Invalid offset bits " ++ show bits
unpackBytes (AJmptbl bits) bs = Left $ "Invalid jmptbl bits " ++ show bits