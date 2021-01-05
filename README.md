# Linux-File-System
A persistent tree-based file system in C using FUSE (File System in User Space), a software that allows you to write a file system by inter- cepting system calls and redirecting them to user-defined handlers. The filesystem is made persistent by writing the in-memory tree structure to secondary storage in a depth first manner.
