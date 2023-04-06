/**
 * @file ssm.c
 * @author lumiknit (aasr4r4@gmail.com)
 * @copyright Copyright (c) 2023 lumiknit
 * @copyright This file is part of ssm.
 */

#include <ssm.h>
#include <ssm_i.h>

static void printVersion() {
  puts(SSM_NAME " - Simple Stack Machine");
  puts(SSM_COPYRIGHT);
  puts("Version: " SSM_VERSION);
}

static void printHelp() {
  printVersion();
  puts("Usage: " SSM_NAME " [options] [file]");
  puts("Options:");
  puts("  -h, --help\t\tPrint this help");
  puts("  -v, --version\t\tPrint version");
  puts("  --stdin\t\tRead ssm object code from stdin");
}

static void loadFromStdin(ssmVM *vm) {
  // Create a buffer
  size_t size = 0;
  size_t capacity = 1024;
  char *buffer = malloc(capacity);
  if(buffer == NULL) {
    fprintf(stderr, "Failed to allocate memory for stdin buffer\n");
    exit(1);
  }
  // Read from stdin
  while(!feof(stdin)) {
    // Read
    size_t read = fread(buffer + size, 1, capacity - size, stdin);
    if(read == 0) {
      break;
    }
    // Resize
    size += read;
    if(size == capacity) {
      capacity *= 2;
      buffer = realloc(buffer, capacity);
      if(buffer == NULL) {
        fprintf(stderr, "Failed to allocate memory for stdin buffer\n");
        exit(1);
      }
    }
  }
  // Load
  ssmLoadString(&vm, buffer, size);
  free(buffer);
}

int main(int argc, char **argv) {
  // Create VM First
  ssmVM vm;
  ssmConfig config;
  ssmLoadDefaultConfig(&config);
  ssmInitVM(&vm, &config);

  // Parse arguments
  int flag_stdin = 0;

  if(argc == 1) {
    // Print help
    printHelp();
    return 0;
  }

  for(int i = 1; i < argc; i++) {
    if(strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
      // Print version
      printVersion();
      return 0;
    } else if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      // Print help
      printHelp();
      return 0;
    } else if(strcmp(argv[i], "--stdin") == 0) {
      // Read from stdin
      flag_stdin = 1;
    } else if(argv[i][0] == '-') {
      // Unknown option
      fprintf(stderr, "Error: Unknown option %s\n", argv[i]);
    } else {
      // Read from file
      ssmLoadFile(&vm, argv[i]);
    }
  }

  // Read from stdin
  if(flag_stdin) {
    loadFromStdin(&vm);
  }

  // Finalize
  ssmFiniVM(&vm);
  return 0;
}