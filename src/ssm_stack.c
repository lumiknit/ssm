/**
 * @file ssm_stack.c
 * @author lumiknit (aasr4r4@gmail.com)
 * @copyright Copyright (c) 2023 lumiknit
 * @copyright This file is part of ssm.
 */

#include <ssm_i.h>

Stack* newStack(size_t size) {
  const size_t bytes = sizeof(Stack) + sizeof(ssmV) * (size - 1);
  Stack* stack = aligned_alloc(SSM_WORD_SIZE, bytes);
  if(stack == NULL) {
    panicf("Failed to alloc stack of size %zu", size);
  }
  stack->size = size;
  stack->top = 0;
  return stack;
}

Stack* extendStack(Stack* stack, size_t size) {
  const size_t bytes = sizeof(Stack) + sizeof(ssmV) * (size - 1);
  stack = realloc(stack, bytes);
  if(stack == NULL) {
    panicf("Failed to alloc stack of size %zu", size);
  }
  stack->size = size;
  return stack;
}

size_t pushStack(Stack* stack, ssmV value) {
  // If stack overflow, return 0
  if (stack->top >= stack->size) {
    return 0;
  }
  stack->data[stack->top++] = value;
  return stack->top;
}

void pushStackForce(Stack** stack_ptr, ssmV value) {
  Stack *stack = *stack_ptr;
  if (stack->top >= stack->size) {
    stack = extendStack(stack, stack->size * 2);
    *stack_ptr = stack;
  }
  stack->data[stack->top++] = value;
}

ssmV popStack(Stack* stack) {
  if(stack->top == 0) {
    panic("Stack underflow");
  }
  return stack->data[--stack->top];
}