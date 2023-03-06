module Opcode where

import Data.List

import qualified Data.ByteString as B
import qualified Data.ByteString.Lazy as BL
import qualified Data.Binary.Get as BG
import qualified Data.Binary.Put as BP

import Arg
import qualified Magic

type Opcode = (String, [Arg.Type])

opcodes :: [Opcode]
opcodes = [
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

opcode :: Int -> Maybe Opcode
opcode i
  | i < length opcodes = Just $ opcodes !! i
  | otherwise = Nothing

opcodeIndex :: String -> Maybe Int
opcodeIndex s = elemIndex s $ map fst opcodes

showOpcode :: Int -> String
showOpcode i = case opcode i of
  Just (name, args) -> name ++ " " ++ unwords (map show args)
  Nothing -> "Unknown opcode"


getOpcode :: BG.Get Opcode
getOpcode = do
  i <- BG.getWord8
  case opcode $ fromIntegral i of
    Just (name, args) -> do
      args <- mapM getByType args
      return (name, args)
    Nothing -> fail $ "Unknown opcode: " ++ show i

unpackOpcode :: B.ByteString -> Either String (Opcode, B.ByteString)
unpackOpcode = runUnpack getOpcode