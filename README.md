# Malloc
A dynamic memory allocator for C programs. 
• mm init: Before calling mm malloc, mm realloc, or mm free, the application program (i.e., the trace-driven driver program that you will use to evaluate your implementation) calls mm init to perform any necessary initialization, such as allocating the initial heap area. The return value should be -1 if there was a problem in performing the initialization, and 0 otherwise.

The driver will call mm init before running each trace (and after resetting the brk pointer). Therefore, your mm init function should be able to reinitialize all state in your allocator each time it is called. In other words, you should not assume that it will only be called once.

• mm malloc: The mm malloc routine returns a pointer to an allocated block with a payload of at least size bytes that begins at an 8-byte aligned address. The entire allocated block should lie within the heap region and should not overlap with any other allocated chunk.

• mm free: The mm free routine frees the block pointed to by ptr. It returns nothing. This routine is only guaranteed to work when the passed pointer (ptr) was returned by an earlier call to mm malloc or mm realloc and has not yet been freed.

• mm realloc: The mm realloc routine returns a pointer to an allocated block with a payload of at least size bytes with the following constraints.

– if ptr is NULL, the effect of the call is equivalent to mm malloc(size);

– if size is equal to zero, the effect of the call is equivalent to mm free(ptr) and the return value is NULL;

– if ptr is not NULL, it must have been returned by an earlier call to mm malloc or mm realloc. The call to mm realloc changes the size of the memory block pointed to by ptr (the old block) to provide a payload of size bytes and returns the address of the new block. The address of the new block might be the same as the old block, or it might be different, depending on your implementation, the amount of internal fragmentation in the old block, and the size of the realloc request. The contents of the new block are the same as those of the old ptr block, up to the minimum of the old and new sizes. Everything else is uninitialized. For example, if the old block is 32 bytes and the new block is 48 bytes, then the ﬁrst 32 bytes of the new block are identical to the ﬁrst 32 bytes of the old block and the last 16 bytes are uninitialized. Similarly, if the old block is 32 bytes and the new block is 16 bytes, then the contents of the new block are identical to the ﬁrst 16 bytes of the old block.
