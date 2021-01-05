# Linux-File-System
A persistent tree-based file system in C using FUSE (File System in User Space), a software that allows you to write a file system by intercepting system calls and redirecting them to user-defined handlers. The filesystem is made persistent by writing the in-memory tree structure to secondary storage in a depth first manner.

This file-system is organised as a tree. Each node of the tree represents either a directory or a regular file. The following information is contained within each node (i.e. inode) -

* The size of the file in bytes
* The number of data blocks currently allocated to that file (applicable to regular files only)
*  Time of last modification, creation and access
*Permissions
* User ID of the owner of the file
* A record holding information of file name, an “is directory” flag, and the number of child nodes (applicable only if the node corresponds to a directory)
* A pointer to the parent node, a pointer to the first child of the node, and a pointer to the node following the current node within the parent directory

The root node of the tree represents the root directory of the filesystem. All nodes originating from a given node are children of that node i.e either sub-directories or files within that directory. Children of a directory are organised as a linked list of nodes. The pointer to the first child of a parent directory is held as a field in the inode of the parent. Each subsequent child is accessed via a pointer stored in the previous child’s inode. Given this structure, access to a file or directory was designed as a traversal of tree nodes, indexed by tokens of the file path. 

The following system calls are implemented - 
* stat
* mkdir
* rmdir
* ls
* touch
* unlink
* open
* mv
* read
* write
* truncate

To run this file system - 
* Create the file system by running 
  * gcc init.c -o create
  * ./create
* To run system commands - 
  * gcc fsys.c -o fsys `pkg-config fuse --cflags --libs`
  * ./fsys -f [mount point]
  * cd into the file system and run the implemented commands

