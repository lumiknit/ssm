#include "ssm_vm.h"
#include "ssm_mem.h"

struct vm {
  mem_t mem;
};

vm_t* new_vm() {
  vm_t *vm = (vm_t*) malloc(sizeof(vm_t));
  init_mem(&vm->mem);
  return vm;
}

void del_vm(vm_t *vm) {
  fin_mem(&vm->mem);
  free(vm);
}

void run_vm(vm_t *vm, op_t *entry) {
}
