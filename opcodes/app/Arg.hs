module Arg where

import Control.Monad

import Data.Int (Int32)
import Data.Word (Word32)
import qualified Data.ByteString as B
import qualified Data.ByteString.Lazy as BL
import qualified Data.Binary.Get as BG
import qualified Data.Binary.Put as BP

import qualified Magic



data Type =
    TInt Int -- Signed int (bits)
  | TUint Int -- Unsigned int (bits)
  | TFloat Int -- Float (bits)
  | TBytes Int -- Bytes (bits of length)
  | TMagic Int -- Magic (bits of instr)
  | TOffset Int -- Offset (bits of offset)
  | TJmptbl Int -- Jump table (bits of offset)
  deriving (Eq)

instance Show Type where
  show (TInt bits) = "i" ++ show bits
  show (TUint bits) = "u" ++ show bits
  show (TFloat bits) = "f" ++ show bits
  show (TBytes bits) = "b" ++ show bits
  show (TMagic bits) = "m" ++ show bits
  show (TOffset bits) = "o" ++ show bits
  show (TJmptbl bits) = "j" ++ show bits

i8 = TInt 8
i16 = TInt 16
i32 = TInt 32
u8 = TUint 8
u16 = TUint 16
u32 = TUint 32
f32 = TFloat 32
b16 = TBytes 16
b32 = TBytes 32
m16 = TMagic 16
o32 = TOffset 32
j32 = TJmptbl 32

data Val =
    VInt Int32
  | VUint Word32
  | VFloat Float
  | VBytes B.ByteString
  | VMagic Int
  | VOffset Int32
  | VJmptbl [Int32]
  deriving (Eq)

instance Show Val where
  show (VInt i) = show i
  show (VUint i) = show i
  show (VFloat f) = show f
  show (VBytes bs) = show bs
  show (VMagic i) = Magic.magics !! i
  show (VOffset i) = show i
  show (VJmptbl is) = show is

runUnpack :: BG.Get Val -> B.ByteString -> Either String (Val, B.ByteString)
runUnpack g b = case BG.runGetOrFail g (BL.fromStrict b) of
  Left (_, offset, err) -> Left msg
    where msg = "Error[" ++ show offset ++ "]: " ++ err
  Right (left, _offset, val) -> Right (val, B.toStrict left)

getByType :: Type -> BG.Get Val

getByType (TInt 8) = VInt . fromIntegral <$> BG.getInt8
getByType (TInt 16) = VInt . fromIntegral <$> BG.getInt16le
getByType (TInt 32) = VInt <$> BG.getInt32le

getByType (TUint 8) = VUint . fromIntegral <$> BG.getWord8
getByType (TUint 16) = VUint . fromIntegral <$> BG.getWord16le
getByType (TUint 32) = VUint <$> BG.getWord32le

getByType (TFloat 32) = VFloat <$> BG.getFloatle

getByType (TBytes 16) = do
  len <- BG.getWord16le
  bytes <- BG.getByteString $ fromIntegral len
  return $ VBytes bytes

getByType (TBytes 32) = do
  len <- BG.getWord32le
  bytes <- BG.getByteString $ fromIntegral len
  return $ VBytes bytes

getByType (TMagic bits) = getByType (TUint bits)

getByType (TOffset bits) = getByType (TInt bits)

getByType (TJmptbl 32) = do
  len <- BG.getWord16le
  offs <- replicateM (fromIntegral len) BG.getInt32le
  return $ VJmptbl offs

getByType ty = fail $ "Invalid arg type " ++ show ty

unpackArg :: Type -> B.ByteString -> Either String (Val, B.ByteString)
unpackArg = runUnpack . getByType

putByType :: Type -> Val -> Either String BP.Put

putByType (TInt 8) (VInt i) = Right $ BP.putInt8 $ fromIntegral i
putByType (TInt 16) (VInt i) = Right $ BP.putInt16le $ fromIntegral i
putByType (TInt 32) (VInt i) = Right $ BP.putInt32le i

putByType (TUint 8) (VUint i) = Right $ BP.putWord8 $ fromIntegral i
putByType (TUint 16) (VUint i) = Right $ BP.putWord16le $ fromIntegral i
putByType (TUint 32) (VUint i) = Right $ BP.putWord32le i

putByType (TFloat 32) (VFloat f) = Right $ BP.putFloatle f

putByType (TBytes 16) (VBytes bs) = Right $ do
  BP.putWord16le $ fromIntegral $ B.length bs
  BP.putByteString bs
putByType (TBytes 32) (VBytes bs) = Right $ do
  BP.putWord32le $ fromIntegral $ B.length bs
  BP.putByteString bs

putByType (TMagic bits) (VMagic i) =
  putByType (TUint bits) (VUint $ fromIntegral i)

putByType (TOffset bits) (VOffset i) =
  putByType (TInt bits) (VInt i)

putByType (TJmptbl 32) (VJmptbl is) = Right $ do
  BP.putWord16le $ fromIntegral $ length is
  mapM_ BP.putInt32le is

putByType ty val =
  Left $ "Invalid arg type " ++ show ty ++ " for " ++ show val

packArg :: Type -> Val -> Either String B.ByteString
packArg ty val = case putByType ty val of
  Left err -> Left err
  Right put -> Right $ BL.toStrict $ BP.runPut put
