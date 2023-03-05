module Main where

import Magic

-- Read the opcodes file
readOpcodes :: IO String
readOpcodes = do
  opcodes <- readFile "opcodes.lst"
  return opcodes

main :: IO ()
main = do
  let x = magicIndexToString 0
  case x of
    Just s -> putStrLn s
    Nothing -> putStrLn "Nothing"
