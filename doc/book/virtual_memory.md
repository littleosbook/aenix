# Virtual memory, an introduction

Virtual memory is an abstraction of physical memory. The purpose of virtual
memory is generally to simplify application development and to let processes
address more memory than what is actually physically present. We don't
want applications messing with the kernel or other applications

In the x86 architecture, virtual memory can be accomplished in two ways:
Segmentation and paging. Paging is by far the most common and versatile
technique, and we'll implement it in chapter 7. Some use of segmentation
is still necessary (to allow for code to execute under different privilege
levels), so we'll set up a minimal segmentation structure in the next chapter.

Managing memory is a big part of what an operating system does. Paging and page
frame allocation (chapter ????) deals with that.

Segmentation and paging is described in the Intel manual [@intel3a], chapter 3
and 4.

## Further reading

- LWN.net has an article on virtual memory: <http://lwn.net/Articles/253361/>
