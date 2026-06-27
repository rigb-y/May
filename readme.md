> May is a memory allocator written in C utilizing the classic Unix `sbrk` interface to extend the program break.
The public interface for May consists of two calls: `mgrant` to grant memory to the user, and `mrel` to 
relinquish that memory when use is complete.

#### Building May

May's ```makefile``` creates a static library file ```lib/libmay.a```.

To use it, link a C file against the library.

```
.
├── libmay.a
├── main.c
└── may.h
```

```c
// main.c

#include "may.h"

int main(int argc, char** argv) { int* p = mgrant(sizeof(int)); ... }
```

```
gcc -c main.c -o main.o
gcc main.o -L. -lmay -o bin
```

Running ```sudo make install``` will install the library `libmay.a` at `/usr/local/lib`, and the header `may.h` at `/usr/local/include`. An optional argument `DESTDIR` can be used to modify these install paths. For example, 
```
sudo make install DESTDIR=$HOME/tmp/
```

installs `$HOME/tmp/usr/local/lib/libmay.a` and `$HOME/tmp/usr/local/include/may.h`. In any case, permissions of  644 (rw--r--r) are set on both files.

After installing, using the library becomes simpler.

```
.
├── main.c
```

```
gcc -c main.c -o main.o
gcc main.o -lmay -o bin
```
  

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

Calling `mrel` on both `NULL` and `nullptr` is a perfectly safe operation. Calling `mrel` on memory already freed is not a safe operation.

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

Initially the program break typically sits just after the space for zero initialized / uninitialized static / globals (.bss). `sbrk` allows us to move this marker, effectively increasing the size of the heap arena.

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
| Block | usable space | Block | usable space | . . .
________________________________________________

```

The `size` field indicates how many usable bytes follow the header. The `free` flag indicates whether or not the block is in use.

#### Free list

Once allocated memory is relinquished, it should be added to the free list. When memory requests are initiated, the allocator checks the free list to see if the request can be fulfilled without allocating new memory.

Note that freeing memory is as simple as recovering the block header, setting the `free` flag to true, and adding the block to the free list.

Recall that in the block structure we maintain a pointer `next`. If we implement the free list as a linked list of Blocks, then this `next` pointer can be used to chain together free blocks in the free list.

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

In other words `s' % A == 0` (or `s' & (A-1) == 0` for the enlightened reader). To be precise,

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
s = Ak,\ k\in \mathbb{Z},
$$

$$
B = A\ell + r,\ \ell,r \in \mathbb{Z},\ 0 < r < A
$$

So,

$$
B + s = Ak + A\ell + r = A(k+\ell) + r
$$

Hence, we see that it is not the case that  

$$
A \mid B+s
.$$

That is, $B+s \not\equiv 0 \pmod{A}$, so $(B+s)$ %  $A \ne 0$, and $B+s$ is not properly aligned.

If both $B$ and $s$ both have remainder zero when divided by $A$, then

$$
A\mid B \ \land \ A\mid s
,$$

so 

$$
A \mid B + s
.$$

Therefore, both $B$ and $s$ must be rounded.


#### Recovering the block header

Given a pointer $p$ to memory that was returned by the allocator, the block header is recovered via

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

 If a header $H$ starts at address $\ell$ with size field $u$, then one past the header sits at $\ell + A_B$. Then, the new header $H^{\prime}$ sits at $\ell + A_B + R_r$. The size field for $H^{\prime}$ is

 $$
 u - R_r - A_B
 .$$

 ```latex
_____________        ___________________________________
H |    u    |   =>   | H |   R_r  | H' | u - R_r - A_B |
_____________        ___________________________________
^                    ^   ^        ^
ℓ                    ℓ   ℓ+A_B    ℓ+A_B+R_r
 ```


#### Coalescing free blocks

If a block $B$ is freed and a neighbor in the arena is also free, that block can absorb its neighbor to create a larger block of free memory.

```
______________________________________________________________________________
| Free block |     space    | B |      space    | Free block |      space    |
______________________________________________________________________________

    ______________________________________________________________________________
 => | B' |                               space                                    |
    ______________________________________________________________________________
```

For any block $B$, getting the address of where its right side neighbor would be (if it existed) is a trivial exercise. We stated above that if a block header sits at address $\ell$, then the address of the next header is $\ell + A_B + R_r$, where $A_B$ is the rounded size of a block header, and $R_r$ is the size of the usable space that the block prefixes. 

However, getting its left neighbor is more difficult. We cannot know just from the information in $B$ where the previous header $B_P$ sits, since that would require knowing how large the usable space that $B_P$ prefixes is.

If all blocks $B$ maintain a pointer to both its left and right neighbors in the arena, then coalescing becomes far easier. A Block structure is now

```C
typedef struct Block {
    size_t size;
    _Bool free;

    // For use in the free list
    struct Block* next;

    struct Block* prev_phys,
                * next_phys;
} Block;
```

#### Alignment Theory

To conclude this discussion on allocators, I will attempt to explain the theory behind the alignment math.

Consider a number $a$ and an alignment $A$. In base two,

$$
    a = \lambda_{n}2^{n} + \lambda_{n-1}2^{n-1} + \ldots + \lambda_{\ell}2^{\ell} + \ldots  
$$
$$
        + \lambda_{\log_{2}(A)}2^{\log_{2}(A)} + \lambda_{\log_{2}(A)-1}2^{\log_{2}(A)-1} 
$$
$$
        +\lambda_{\log_{2}(A)-2}2^{\log_{2}(A)-2} + \ldots + \lambda_{1}2^{1} + \lambda_{0}2^{0}
.$$

Note that $\lambda_{i} \in \left\{0,1\right\}$. Observe that

$$
A = \lambda_{\log_{2}(A)}2^{\log_{2}(A)}
$$

when $\lambda_{\log_{2}(A)} = 1$. Further observe that all terms that come before this term are divisible by $A$. These terms are in the form

$$
\lambda_{\log_{2}(A)+k}2^{\log_{2}(A) + k}
$$

for $k \in \mathbb{Z}_{>0}$, and $\log_{2}(A)+k \leq n$. Observe that if $\lambda_{\log_{2}{(A)}+k} = 1$,

$$
\frac{2^{\log_{2}{(A)}+k}}{2^{\log_{2}{(A)}}} = 2^{k} \in \mathbb{Z}
,$$

and if $\lambda_{\log_{2}{(A) + k}} = 0$,

$$
\frac{0}{2^{\log_{2}{(A)}}} = 0 
.$$

Thus, $a$ will only have a remainder after being divided by $A$  when all terms of the form

$$
\lambda_{\log_{2}{(A) - j}}2^{\log_{2}{(A)-j}} 
$$

have $\lambda_{\log_{2}{(A)-j}} =0$, since none of these terms are divisible by $A$. Note that $j \in \mathbb{Z}_{>0}$ and $\log_{2}{(A) - j} \geq 0$

This is precisely why the check

```c
a & (A-1) == 0
```

Checks if $a$ has remainder zero when divided by $A$. Now, consider

```
~(A-1) 
```

This is the $n$ bit number with all terms greater than or equal to $2^{\log_{2}{(A)}}$ non zero, and all terms less than $2^{\log_{2}{(A)}}$ zero. In essence, all bits left of and including the $(\log_{2}{(A)} + 1)$th bit are one, and all bits to the right are zero. For example, take $A = 16$, then 

$$
\sim(A - 1) = \sim(15) = 0b1111\ldots11110000
$$

Here $\log_2{(16)}  =4$, observe the bit pattern.

So, the bitwise and of a number $a$ with this mask turns off those bottom four bits. Since we removed the bits that would cause a remainder when we take $a$ divided by $A$, we rounded $a$ down to the nearest boundary such that it is $A$ aligned.

Now, adding $A-1$ to $a$ moves the address such that it is inside the next highest boundary, then we take the bitwise and of the negation of the alignment -1 (see above). This rounds it down to the bottom of that next highest boundary. Thus, a round up of the original boundary.
