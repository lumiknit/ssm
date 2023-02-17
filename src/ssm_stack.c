/**
 * @file ssm_stack.c
 * @author lumiknit (aasr4r4@gmail.com)
 * @copyright Copyright (c) 2023 lumiknit
 * @copyright This file is part of ssm.
 */

#include <ssm_i.h>

Stack* newStack(size_t size, int r2l) {
  const size_t bytes = sizeof(Stack) + sizeof(ssmV) * (size - 1);
  Stack* stack = aligned_alloc(SSM_WORD_SIZE, bytes);
  if(stack == NULL) {
    panicf("Failed to alloc stack of size %zu", size);
  }
  stack->size = size;
  stack->top = r2l ? size : 0;
  return stack;
}

Stack* extendStackToRight(Stack* stack, size_t size) {
  const size_t bytes = sizeof(Stack) + sizeof(ssmV) * (size - 1);
  stack = realloc(stack, bytes);
  if(stack == NULL) {
    panicf("Failed to alloc stack of size %zu", size);
  }
  stack->size = size;
  return stack;
}

Stack* extendStackToLeft(Stack* stack, size_t size) {
  Stack *new_stack = newStack(size, 1);
  size_t move_words = stack->size - stack->top;
  new_stack->top -= move_words;
  memcpy(new_stack->vals + new_stack->top,
         stack->vals + stack->top,
         move_words * sizeof(ssmV));
  freeStack(stack);
  return new_stack;
}

size_t pushStack(Stack* stack, ssmV value) {
  // If stack overflow, return 0
  if (stack->top >= stack->size) {
    return 0;
  }
  stack->vals[stack->top++] = value;
  return stack->top;
}

void pushStackForce(Stack** stack_ptr, ssmV value) {
  Stack *stack = *stack_ptr;
  if (stack->top >= stack->size) {
    stack = extendStackToRight(stack, stack->size * 2);
    *stack_ptr = stack;
  }
  stack->vals[stack->top++] = value;
}

ssmV popStack(Stack* stack) {
  if(stack->top == 0) {
    panic("Stack underflow");
  }
  return stack->vals[--stack->top];
}

size_t pushStackR(Stack* stack, ssmV value) {
  // R is unchecked
  stack->vals[--stack->top] = value;
  return stack->top;
}

ssmV popStackR(Stack* stack) {
  // R is unchecked
  return stack->vals[stack->top++];
}