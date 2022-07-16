#ifndef __SSM_VM_H__
#define __SSM_VM_H__

struct vm;
typedef struct vm vm_t;

vm_t* create_vm();
op_t* append_bytecode(vm_t *vm, op_t *bytecode);

void run_vm(vm_t *vm, op_t *entry);

#endif
