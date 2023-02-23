/**
 * @file ssm.c
 * @author lumiknit (aasr4r4@gmail.com)
 * @copyright Copyright (c) 2023 lumiknit
 * @copyright This file is part of ssm.
 */

#include <ssm.h>
#include <ssm_i.h>

int main() {
  ssmVM vm;
  ssmConfig config;
  ssmLoadDefaultConfig(&config);
  ssmInitVM(&vm, &config);
  ssmRunVM(&vm, 0);

  unimplemented();

  ssmFiniVM(&vm);
  return 0;
}