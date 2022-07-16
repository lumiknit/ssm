#ifndef __SSM_VM_H__
#define __SSM_VM_H__

#include "ssm_val.h"
#include "ssm_code.h"

struct vm;
typedef struct vm vm_t;

vm_t* new_vm();
void del_vm(vm_t*);

void run_vm(vm_t *vm, op_t *entry);

#endif
