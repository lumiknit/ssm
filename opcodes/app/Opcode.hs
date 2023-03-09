module Opcode where

import Control.Monad
import Data.List

import qualified Data.ByteString as B
import qualified Data.ByteString.Lazy as BL
import qualified Data.Binary.Get as BG
import qualified Data.Binary.Put as BP

import Arg
import qualified Magic

type OpSpec = (String, [Arg.Type])

opSpecs :: [OpSpec]
opSpecs = [
  -- No op
  ("NOP", []),
  -- Header
  ("HEADER", [u32, u32, u32]),
  ("HALT", []),
  -- Stack
  ("POP", [u16]),
  ("PUSH", [u16]),
  ("PUSHBP", [u16]),
  ("PUSHAP", [i16]),
  ("POPSET", [u16]),
  -- Literal
  ("PUSHI", [i32]),
  ("PUSHF", [f32]),
  -- Function
  ("PUSHFN", [o32]),
  -- Global
  ("PUSHGLOBAL", [u32]),
  ("POPSETGLOBAL", [u32]),
  -- Tuple
  ("PUSHISLONG", [u16]),
  -- Short tuple
  ("TUP", [u16, u16]),
  ("TAG", [u8]),
  ("PUSHTAG", [u16]),
  ("PUSHLEN", [u16]),
  ("PUSHELEM", [u16, u16]),
  -- Long tuple
  ("LONG", [b32]),
  ("PACK", [u32]),
  ("SETBYTE", [u16, u16]),
  ("PUSHLONGLEN", [u16]),
  ("PUSHBYTE", [u16]),
  ("JOIN", []),
  ("SUBLONG", []),
  ("LONGCMP", []),
  -- Call
  ("APP", [u32]),
  ("RET", [u32]),
  ("RETAPP", [u32]),
  -- Int Arithmetic
  ("INTADD", []),
  ("INTSUB", []),
  ("INTMUL", []),
  ("UINTMUL", []),
  ("INTDIV", []),
  ("UINTDIV", []),
  ("INTMOD", []),
  ("UINTMOD", []),
  ("INTUNM", []),
  ("INTSHL", []),
  ("INTSHR", []),
  ("UINTSHR", []),
  ("INTAND", []),
  ("INTOR", []),
  ("INTXOR", []),
  ("INTNEG", []),
  ("INTLT", []),
  ("INTLE", []),
  -- Float Arithmetic
  ("FLOATADD", []),
  ("FLOATSUB", []),
  ("FLOATMUL", []),
  ("FLOATDIV", []),
  ("FLOATUNM", []),
  ("FLOATLT", []),
  ("FLOATLE", []),
  -- Comparison
  ("EQ", []),
  ("NE", []),
  -- Branch
  ("JMP", [o32]),
  ("BEQ", [o32]),
  ("BNE", [o32]),
  ("BTAG", [u16, o32]),
  ("JTAG", [j32]),
  -- Magic
  ("MAGIC", [m16]),
  -- Literal Marker
  ("XFN", [u16, u32]) ]

opSpec :: Int -> Maybe OpSpec
opSpec i
  | i < length opSpecs = Just $ opSpecs !! i
  | otherwise = Nothing

opSpecIndex :: String -> Maybe Int
opSpecIndex s = elemIndex s $ map fst opSpecs

showOpSpec :: Int -> String
showOpSpec i = case opSpec i of
  Nothing -> "Unknown opcode"
  Just (name, argTypes) -> name ++ " " ++ unwords (map show argTypes)

data OpCode = OpCode Int [Val]
  deriving (Eq)

instance Show OpCode where
  show (OpCode i args) = opName ++ " " ++ unwords (map show args)
    where opName = case opSpec i of
            Nothing -> "Unknown"
            Just (name, _) -> name

getOpcode :: BG.Get OpCode
getOpcode = do
  i <- BG.getWord8
  let i = fromIntegral i
  case opSpec i of
    Nothing -> fail $ "Unknown opcode: " ++ show i
    Just (name, args) -> do
      args <- mapM getByType args
      return $ OpCode i args

unpackOpcode :: B.ByteString -> Either String (OpCode, B.ByteString)
unpackOpcode = runUnpack getOpcode

putOpcode :: OpCode -> Either String BP.Put
putOpcode (OpCode i args) = do
  case opSpec i of
    Nothing -> Left $ "Unknown opcode: " ++ show i
    Just (name, argTypes) ->
      case f (zip argTypes args) (Right []) of
        Left e -> Left e
        Right v -> Right $ do
          BP.putWord8 $ fromIntegral i
          sequence_ v
      where f [] v = v
            f _ (Left v) = Left v
            f ((ty, a) : xs) (Right v) = case putByType ty a of
              Right p -> Right $ p : v
              Left e -> Left e

packOpcode :: OpCode -> Either String B.ByteString
packOpcode code = case putOpcode code of
  Left e -> Left e
  Right p -> Right $ B.toStrict $ BP.runPut p