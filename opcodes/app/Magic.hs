module Magic where

import qualified Data.ByteString as B
import Data.List

magics :: [String]
magics = [
  "NOP",
  "HALT",
  "NEWVM",
  "NEWPROCESS",
  "VMSELF",
  "VMPARENT",
  "EVAL",
  "HALTED",
  "SENDMSG",
  "RECVMSG",
  "FOPEN",
  "FCLOSE",
  "FFLUSH",
  "FREAD",
  "FWRITE",
  "FTELL",
  "FSEEK",
  "FEOF",
  "STDREAD",
  "STDWRITE",
  "STDERROR",
  "REMOVE",
  "RENAME",
  "TMPFILE",
  "READFILE",
  "WRITEFILE",
  "MALLOC",
  "FREE",
  "SRAND",
  "RAND",
  "ARG",
  "ENV",
  "EXIT",
  "SYSTEM",
  "PI",
  "E",
  "ABS",
  "SIN",
  "COS",
  "TAN",
  "ASIN",
  "ACOS",
  "ATAN",
  "ATAN2",
  "EXP",
  "LOG",
  "LOG10",
  "LN",
  "MODF",
  "POW",
  "SQRT",
  "CEIL",
  "FLOOR",
  "FABS",
  "FMOD",
  "CLOCK",
  "TIME",
  "CWD",
  "ISDIR",
  "ISFILE",
  "MKDIR",
  "RMDIR",
  "CHDIR",
  "FILES",
  "FFILOAD" ]

magicStringToIndex :: String -> Maybe Int
magicStringToIndex s = elemIndex s magics

magicIndexToString :: Int -> Maybe String
magicIndexToString i =
  if i < length magics
  then Just (magics !! i)
  else Nothing