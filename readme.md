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
________________________________________________
| Block | useable space | Block | usable space | . . .
________________________________________________

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

#### Fragmentation

* **Internal fragmentation**: A block was allocated that is bigger than the requested size, so space goes unused.
* **External fragmentation**: There exists enough total memory to fulfill the request, but it is scattered amongst blocks that are too small for the request.

#### Address aligning

In order to guarantee that addresses are properly aligned to the hardware's liking, we must ensure that request sizes are rounded up to the nearest boundary. But, which boundary do we use? In C we can use `max_align_t`, which is given by `stddef.h` and represents a type with the largest alignment requirement of any type. So,

```C
size_t A = _Alignof(max_align_t);
```

gives us that alignment. We can then create a simple function that takes a size $s$ and rounds it to $s^{\prime}$ such that

$$
s^{\prime} \equiv 0 \pmod{A}.
$$

In other words `s' % A == 0` (or `s' & (A-1) == 0`) for the enlightened reader. To be precise,

$$
s^{\prime} = \min\{\lambda \geq s : \lambda \equiv 0 \pmod{A}\}
$$

The function is simply

```C
size_t alignup(size_t s) {
    return (s + A-1) & ~(A-1);
}
```

However, it is not enough to round the requested size, we must also round the size of the block header. Call the size of the block header $B$, and consider a situation in which $A\mid s$, but $A\nmid B$. Some rudimentary number theory tells us that

$$
\begin{align*}
s &= Ak,\; k\in \mathbb{Z}, \\
B &= A\ell + r,\; \ell,r \in \mathbb{Z},\; 0 < r < A
.\end{align*}
$$
So,

$$
B + s = Ak + A\ell + r = A(k+\ell) + r
$$

Hence, we see that it is not the case that  

$$
A \mid B+s
.$$

That is, $B+s \not\equiv 0 \pmod{A}$, so $(B+s)\; \%\;  A \ne 0$, and $B+s$ is not properly aligned.

If both $B$ and $s$ both have remainder zero when divided by $A$, then

$$
A\mid B \; \land \; A\mid s
,$$

so 

$$
A \mid B + s
.$$

Therefore, both $B$ and $s$ must be rounded.


#### Recovering the block header

Given a pointer $p$ to memory that was returned by the allocator to the user, the block header is recovered via

```C
(Block*)((char*)p - ALIGNED_HEADER_SIZE);
```

#### Splitting blocks

Consider a case in which we have a free block that describes a $256$ byte region of the heap arena.

```
______________________
| Header | 256 bytes |
______________________
```

But, suppose that a caller requests only 16 bytes. It would be unwise to hand over  this entire 256 byte chunk. Instead, we can split the chunk into two distinct blocks. The first chunk fulfills the request exactly (after rounding). Then, the second chunk a new block header followed by the remaining space.

```
_________________________________________________
| Header | 16 bytes  | Header | remaining space |
_________________________________________________
```

The first chunk is what is returned by the allocator to fulfil the 16 byte request.

Suppose a user requests $r$ bytes, and the `first-fit` protocol finds a block of size $u$.

Impose a minimum size $M_s$ on requestable space. A natural choice for $M_s$ is the size of the alignment criteria $A$.
 Call the rounded size of a block `alignup(sizeof(Block))` $A_B$,
 and rounded requested size `alignup(r)` $R_{r}$. 

 If 
 $$
 R_r + A_B + M_s \leq u
 ,$$
 then we split the block.

 If a header $H$ starts at address $\ell$ with size field $u$, then 
 one past the header sits at $\ell + A_B$. 
 So, the new header $H^{\prime}$ sits at $\ell + A_B + R_r$. 
 The size field for $H^{\prime}$ is

 $$
 u - R_r - A_B
 .$$

 ```latex
________________________________
 | H | \(R_r\) | H' | u - R_r - A_B |
 ________________________________
 ℓ
 ```
