> May is a memory allocator written in C utilizing the classic system call wrapper `sbrk` to extend the program break.
The public interface for May consists of two calls: `mgrant` to grant memory to the user, and `mrel` to 
relinquish that memory when use is complete.
  

#### Allocating memory
Users in need of runtime allocated memory can call `mgrant` (memory grant).
```C
void* mgrant(size_t request);
```

If the request can be fulfilled, a pointer to the start of the allocated memory is returned. Otherwise, `NULL` is returned.

#### Relinquishing memory

Users no longer in need of their `mgrant` allocated memory can call `mrel` (memory release) to relinquish that memory.

```c
void mrel(void* mem);
```

Calling `mrel` on both `NULL` and `nullptr` is a perfectly safe operation. Calling `mrel` on memory already freed is a `nop`.

---

#### sbrk and the  program break

The program break marks the end of the heap arena. Consider the virtual memory layout of a program.

```
High addresses
+-----------------------------+
| stack                       |
+-----------------------------+
|                             |
| Unmapped / guard space      |
|                             |
+-----------------------------+
| Memory-mapped region        |
+-----------------------------+
| Heap                        |
+-----------------------------+ <- Initial program break
| BSS segment                 |
+-----------------------------+
| Data segment                |
+-----------------------------+
| Read-only data              |
+-----------------------------+
| Text segment                |
+-----------------------------+
Low addresses
```

Initially the program break sits just after the space for initialized static / globals (.bss). `sbrk` allows us to move this marker, effectively increasing the size of the heap arena.

```c
void* sbrk(intptr_t inc);
```

`sbrk` returns the old program break before it was increased or `(void*)-1` on error. Observe that 

```c
sbrk(0);
```

merely returns where the program break currently sits.

#### Block headers

All memory locations returned by the allocator are prefixed by a secret block structure. The purpose of the block structure is to provide metadata about each allocation. A simple header is 

```C
typedef struct Block {
    size_t size;
    _Bool free;
    struct Block* next;
} Block;
```

The heap arena is then logically partitioned in the following way

```
------------------------------------------------
| Block | useable space | Block | usable space | . . .
------------------------------------------------

```

The `size` field indicates how many useable bytes follow the header. The `free` flag indicates whether or not the block is in use.

#### Free list

Once allocated memory is relinquished, it should be added to the free list. When memory requests are initiated, the allocator checks the free list to see if the request can be fulfilled without allocating new memory.

Note that freeing memory is as simple as recovering the block header, setting the `free` flag to  false, and adding the block to the free list.

##### Procuring blocks from the free list

We can use one of:

1. **First-fit**: Return the first block that has enough space to fulfill the request.
1. **Best-fit**: Return the block that causes the least amount of internal fragmentation.
1. **Worst-fit**: Return the block that causes the most amount of internal fragmentation.
1. **Next-fit**: Start searching from the place where the previous search ended.

**Note**: May currently uses *first-fit* (for the time being).

##### Fragmentation

* **Internal fragmentation**: A block was allocated that is bigger than the requested size, so space goes unused.
* **External fragmentation**: There exists enough total memory to fulfill the request, but it is scattered amongst blocks that are too small for the request.

###
