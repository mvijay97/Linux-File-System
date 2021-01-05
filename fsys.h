#define MAX_CHAR 256
#define BLOCK_SIZE 512
#define MAX_BLOCKS 5
#define N_DATA_BLOCKS 50
#define MAX_INODES 20


#include <sys/stat.h>
#include <sys/types.h>

typedef struct dirent{
	char file_name[MAX_CHAR]; //File name
	int is_dir; //Flag set to 1 if the inode corresponds to a directory, 0 if it is a file
	int n_child; //Number of subdirectories and files
}dirent;

typedef struct inode{
	mode_t    st_mode;    
	nlink_t   st_nlink;    
	uid_t     st_uid;      
	off_t     st_size;    
	time_t    st_access_time;   
	time_t    st_mod_time;   
	time_t    st_create_time; 
	blksize_t st_blksize;
	blkcnt_t  st_blocks;                   
	struct dirent metadata;
	struct inode* parent;
	struct inode* first_child;
	struct inode* next_child;
	int offset[MAX_BLOCKS];
}inode;

inode* create_node();
void init_dir(inode* dir,const char* path);
void setfileattr(inode *child, char *filename);
int traverse_path(const char* path, inode** found_at);
void mkfs();