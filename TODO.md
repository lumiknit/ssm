# TODO

## GC

- Change major heap into free list
  - Instead of stack, use the default allocator with an object chain
    - (Place next major object pointer at \[-1\] of object)
  - Instead of scan major stack, scan the object chain
  - Add crieria to determine when the major scan needs
  - Since major objects are not moved, only minor's pointer entries may be rewritten.
    - Write barrier?
- Create constant pool
  - As a stack
  
  