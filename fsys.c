#define FUSE_USE_VERSION 30
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "fsys.h"


FILE* disk;
int n_nodes;

int* bitmap; //INIT
int start_of_data_region; //INIT
int start_of_inode_region;

inode* root=NULL;
time_t T;

inode* create_node()
{
	n_nodes++;
	inode* new_node = (inode*)malloc(sizeof(inode));
	if(new_node)
	{
		new_node->parent=new_node->first_child=new_node->next_child=NULL;
		for(int i=0;i<MAX_BLOCKS;i++)
			new_node->offset[i]=-1;
		return(new_node);
	}
	printf("ALLOCATION ERROR\n");
	return(NULL);
}

inode* allocate()
{
	inode* temp = (inode*)malloc(sizeof(inode));
	if(temp)
		return(temp);
	printf("AVAILABLE SPACE INSUFFICIENT\n");
	exit(0);
}

void init_dir(inode* dir,const char* path)
{
	time(&T);
	dir->st_access_time=T;
	dir->st_create_time=T;
	dir->st_mod_time=T;
	dir->st_uid=getuid();
	dir->st_mode=S_IFDIR|0755;
	dir->st_size=BLOCK_SIZE;
	dir->st_blocks=0;
	dir->st_nlink=2;
	dir->metadata.n_child=0;
	dir->metadata.is_dir=1;
	strcpy(dir->metadata.file_name,path);
	printf("INIT_DIR: %s\n",dir->metadata.file_name );
}

void setfileattr(inode *child, char *filename)
{
	child->metadata.is_dir = 0;
	child->metadata.n_child=0;
	strcpy(child->metadata.file_name,filename);
	child->st_size = 0;
	child->st_nlink = 1;
	child->st_uid = getuid();
	child->st_mode = S_IFREG | 0644;
	child->st_blocks = 0;
	time(&T);
	child->st_access_time = T;
	child->st_mod_time = T;
	child->st_create_time = T;
}

void mkfs()
{
	root=create_node();
	time(&T);
	root->st_access_time=T;
	root->st_create_time=T;
	root->st_mod_time=T;
	root->st_uid=getuid();
	root->st_mode=S_IFDIR|0755;
	root->st_size=4096;
	root->st_blocks=8;
	root->st_nlink=2;

	root->metadata.is_dir=1;
	strcpy(root->metadata.file_name,"/");
	for(int i=0;i<N_DATA_BLOCKS;i++)
		bitmap[i]=0;
}

int traverse_path(const char* path, inode** found_at)
{
	printf("TRAVERSE_PATH: %s\n",path );
	char* temp=(char*)malloc(MAX_CHAR);
	strcpy(temp, path);
	if(strcmp(path,"/")==0 || strcmp(path,"")==0)
	{
		*found_at=root;
		free(temp);
		return(1);
	}
	char* token=strtok(temp,"/");
	inode* parent=root;
	inode* child=NULL;
	while(token!=NULL)
	{
		int found=0;
		child=parent->first_child;
		while(child!=NULL)
		{
			if(strcmp(token,child->metadata.file_name)==0)
			{
				found=1;
				break;	
			}
			child=child->next_child;
		}
		if(found==0)
		{
			*found_at=NULL;
			free(temp);
			return(0);
		}
		parent=child;
		token=strtok(NULL,"/");
	}
	*found_at=child;
	free(temp);
	return(1);
}

char* allocate_data_block()
{
	char* new_block = (char*)malloc(BLOCK_SIZE);
	if(new_block){
		memset(new_block,0,BLOCK_SIZE);
		return(new_block);
	}
	else
	{
		printf("BLOCK ALLOCATION ERROR\n");
		return(NULL);
	}
}

static int fgetattr(const char * file_path, struct stat * st)
{
	printf("FGETATTR: %s\n",file_path );
	memset(st,0,sizeof(struct stat));
	inode* node;
	if(traverse_path(file_path,&node))
	{
		//*st=node->st;
		st->st_mode = node->st_mode; 
		st->st_nlink = node->st_nlink;
		st->st_uid = node->st_uid;
		st->st_size = node->st_size;
		st->st_atime = node->st_access_time;
		st->st_mtime = node->st_mod_time;
		st->st_ctime = node->st_create_time;
		st->st_blksize = node->st_blksize;
		st->st_size = node->st_size;
		return(0);
	}
	return(-ENOENT);
}

static int fmkdir(const char * file_path, mode_t mode)
{
	printf("MKDIR: %s\n",file_path);
	inode* parent;
	char* path = (char*)malloc(MAX_CHAR);
	char* ptr =strrchr(file_path,'/');
	strncpy(path,file_path,ptr-file_path);
	path[ptr-file_path]='\0';
	if(traverse_path(path,&parent))
	{
		inode* dir = create_node();
		init_dir(dir,strrchr(file_path,'/')+1);
		dir->parent=parent;
		dir->next_child=parent->first_child;
		parent->first_child=dir;
		parent->metadata.n_child+=1;
		printf("MKDIR RESULT: %d\n",traverse_path(file_path,&parent));
		free(path);
		return(0);
	}
	free(path);
	return(-ENOENT); // parent directory not found
}

static int freaddir(const char *file_path, void *buf, fuse_fill_dir_t filler,off_t offset, struct fuse_file_info *fi)
{
	printf("READDIR: %s\n",file_path );
	inode* dir;
	if(traverse_path(file_path,&dir))
	{
		filler(buf,".",NULL,0);
		filler(buf,"..",NULL,0);
		dir=dir->first_child;
		while(dir!=NULL)
		{
			filler(buf,dir->metadata.file_name,NULL,0);
			dir=dir->next_child;
		}
		return(0);
	}
	return(-ENOENT);
}

static int frmdir(const char * file_path)
{
	inode* node;
	if(traverse_path(file_path,&node))
	{
		if(node->metadata.n_child==0)
		{
			inode* parent=node->parent;
			if(parent->first_child==node)
			{
				inode* temp=parent->first_child;
				parent->first_child=temp->next_child;
				free(temp);
			}
			else
			{
				inode* cur_child=parent->first_child->next_child;
				inode* prev_child=NULL;
				while(cur_child!=node){
					prev_child=cur_child;
					cur_child=cur_child->next_child;
				}
				prev_child->next_child=cur_child->next_child;
				free(cur_child);
			}
			parent->metadata.n_child-=1;
			return(0);
		}
		return(-ENOTEMPTY); //directory is not empty
	}
	return(-ENOENT); //directory does not exist
}

void remnode(inode *node)
{
	inode *parent, *temp;
	parent = node->parent;
	if(parent==NULL) return;
	if(parent->first_child == node)
		parent->first_child = node->next_child;
	else
	{
		//search for temp wrt to parent
		temp = parent->first_child;
		while(temp!=NULL)
		{
			if(temp->next_child == node)
			{
				temp->next_child = node->next_child;
				break;
			}
			temp = temp->next_child;
		}
	}
	//reset bitmap of this files' blocks
	for(int i = 0;i<MAX_BLOCKS;i++)
	{
		data_block = node->offset[i];
		if(bitmap[data_block]==1)
			bitmap[data_block]=0;
	}

	parent->st_nlink -= 1;
	//change parent's timestamps
	time_t tim;
	time(&tim);
	parent->st_create_time = T;
	parent->st_mod_time = T;
}

static int unlinkfile(const char *path)
{
	inode *node = NULL;
	int exists = traverse_path(path,&node);
	if(!exists) return -ENOENT;
	if(node->metadata.is_dir == 1) return -EISDIR;
	remnode(node);
	//free the inode
	//long memfreed = sizeof(inode);
	/*if(node->data != NULL)
	{
		memfreed = memfreed + node->st.st_size;
		free(node->data);
		node->data = NULL;
	}*/
	free(node);
	//available_mem += memfreed;
	return 0;
}

static int fcreate(const char *file_path, mode_t mode,struct fuse_file_info *fi)
{
	printf("CALL TO CREATE\n");
	inode* parent;
	if(traverse_path(file_path,&parent))
		return(-EEXIST);
	
	char* path = (char*)malloc(MAX_CHAR);
	char* ptr =strrchr(file_path,'/');
	strncpy(path,file_path,ptr-file_path);
	path[ptr-file_path]='\0';
	
	if(traverse_path(path,&parent))
	{
		inode* file = create_node();
		if(file==NULL)
			return(0);
		else
		{
			setfileattr(file,ptr+1);
			file->next_child=parent->first_child;
			file->parent=parent;
			parent->first_child=file;
			parent->metadata.n_child+=1;
			time(&T);
			parent->st_create_time = T;
			parent->st_mod_time = T;
			return(0);
		}
	}
	return(-ENOENT);
}

static int find_free_block()
{
	for(int i=0;i<N_DATA_BLOCKS;i++)
	{
		if(bitmap[i]==0){
			bitmap[i]=1;
			return(i);
		}
	}
	return(-1);
}


static int f_write(const char *file_path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	// printf("CALL TO WRITE %d %d %s\n",size,offset,buf);
	inode* node;
	if(traverse_path(file_path,&node))
	{
		disk=fopen("persistent_st","rb+");
		if(node->st_size==0) //if no data blocks have been allocated
		{
			int data_blk=find_free_block(); //find index of a free data block
			if(data_blk==-1)
			{
				printf("NO FREE BLOCKS\n");
				return(0);
			}

			node->st_blocks=1; // set blocks to 1
			node->st_size=size; //set size to the number of bytes written
			fseek(disk,start_of_data_region+data_blk*BLOCK_SIZE,SEEK_SET);
			fwrite(buf,1,size,disk);
			node->offset[0]=data_blk;
			fclose(disk);
			return(size);
		}
		
		else //some data has already been written to file
		{
			int last_block_written;
			for(int i=0;i<MAX_BLOCKS;i++) //find the last block allocated
			{
				if(node->offset[i]!=-1)
					last_block_written=i;
			}

			int freespace = node->st_blocks*BLOCK_SIZE-node->st_size;
			if(freespace>size) // if the data to be written can be accomodated in the current block
			{
				fseek(disk,start_of_data_region+last_block_written*BLOCK_SIZE+(BLOCK_SIZE-freespace),SEEK_SET);
				fwrite(buf,1,size,disk);
				node->st_size+=size;
				fclose(disk);
				return(size);
			}

			else // new blocks must be allocated
			{
				int rem_size=size;
				fseek(disk,start_of_data_region+last_block_written*BLOCK_SIZE+(BLOCK_SIZE-freespace),SEEK_SET);
				fwrite(buf,1,freespace,disk);
				last_block_written++;
				rem_size-=freespace;
				
				while(rem_size>0)
				{
					int data_blk=find_free_block();
					if(data_blk==-1)
					{
						printf("ALLOCATION ERROR\n");
						fclose(disk);
						return(0);
					}
					
					node->offset[last_block_written]=data_blk; //update data pointers in inode					
					fseek(disk,start_of_data_region+data_blk*BLOCK_SIZE,SEEK_SET);
					if(rem_size>BLOCK_SIZE)
					{
						fwrite(buf,1,BLOCK_SIZE,disk);	
						rem_size-=BLOCK_SIZE;
					}
					
					else
					{
						fwrite(buf,1,rem_size,disk);
						rem_size=0;
					}
					node->st_blocks+=1;
					last_block_written++;
				}
				node->st_size+=size;
				fclose(disk);
				return(size);
			}
		}
	}

	return(-ENOENT);
}

static int f_read(const char *file_path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	inode* node;
	if(traverse_path(file_path,&node))
	{
		if(node->metadata.is_dir==1)
			return(-EISDIR);
		disk=fopen("persistent_st","rb");
		
		int rem_size=node->st_size;
		if(rem_size<BLOCK_SIZE)
		{
			fseek(disk,start_of_data_region+node->offset[0]*BLOCK_SIZE,SEEK_SET);
			fread(buf,1,size,disk);
			fclose(disk);
			return(size);
		}
		else
		{
			int current_offset=0;
			while(rem_size>0)
			{
				if(rem_size>BLOCK_SIZE)
				{
					fseek(disk,start_of_data_region+node->offset[current_offset]*BLOCK_SIZE,SEEK_SET);
					fread(buf+(current_offset*BLOCK_SIZE),1,BLOCK_SIZE,disk);
					current_offset++;
					rem_size-=BLOCK_SIZE;
				}
				else
				{
					fseek(disk,start_of_data_region+node->offset[current_offset]*BLOCK_SIZE,SEEK_SET);
					fread(buf+(current_offset*BLOCK_SIZE),1,rem_size,disk);
				}
			}
			fclose(disk);
			return(size);
		}
	}
	return(-ENOENT);
}

static int f_open(const char *file_path, struct fuse_file_info *fi)
{
	inode* node;
	if(traverse_path(file_path,&node))
		return(0);
	return(-ENOENT);
}

void write_to_file(inode* parent)
{
	fwrite(parent,sizeof(struct inode),1,disk);
	printf("%s\n",parent->metadata.file_name );
	inode* temp = parent->first_child;
	while(temp)
	{
		if(temp->metadata.is_dir)
		{
			if(temp->metadata.n_child!=0)
				write_to_file(temp);
			else{
				printf("serialise %s\n",temp->metadata.file_name );
				fwrite(temp,sizeof(struct inode),1,disk);
			}
		}
		else{
			fwrite(temp,sizeof(struct inode),1,disk);
		}
		temp=temp->next_child;
	}
}

void f_destroy(void* private_data)
{
	disk=fopen("persistent_st","rb+");
	fwrite(&n_nodes,sizeof(int),1,disk);
	fwrite(bitmap,sizeof(int)*N_DATA_BLOCKS,1,disk);
	write_to_file(root);
	fclose(disk);
}

void read_from_file(inode* parent)
{
	int n_child=parent->metadata.n_child;
	printf("%d\n",n_child);
	inode* temp;
	inode* current;
	
	if(n_child>0)//initially checking if root has children at all then go inside if
	{
		temp=allocate();
		fread(temp,sizeof(struct inode),1,disk);
		temp->parent=parent;
		parent->first_child=temp;

		printf("first %s\n",parent->first_child->metadata.file_name);

		if(temp->metadata.n_child>0)//if isn't exec when the node has no children or when its a file
		{
			printf("children\n");
			read_from_file(temp);//recursively add current node's childrens' inodes into tree by reading from pers
		}
		current=parent->first_child;
	}

	for(int i=1;i<n_child;i++)//no in each level add the sibling nodes to tree in bfs manner
	{
		temp=allocate();
		fread(temp,sizeof(struct inode),1,disk);
		current->next_child=temp;
		printf("second %s %s\n",current->parent->metadata.file_name, parent->first_child->next_child->metadata.file_name );//second string?

		temp->parent=parent;

		if(temp->metadata.n_child>0)
			read_from_file(temp);
		current=temp;
	}
}

static struct fuse_operations ops={
	.getattr = fgetattr,  
	.mkdir = fmkdir,
	.readdir = freaddir,
	.rmdir = frmdir,
	.create = fcreate,
	.unlink = unlinkfile,
	.write = f_write,
	.read = f_read,
	.open = f_open,
	.destroy= f_destroy
};



int main(int argc, char *argv[])
{
	bitmap=(int*)malloc(sizeof(int)*N_DATA_BLOCKS);
	disk=fopen("persistent_st","rb");
	fread(&n_nodes,sizeof(int),1,disk);//superblock block read into tree from persistent storage
	fread(bitmap,sizeof(int)*N_DATA_BLOCKS,1,disk);//data bitmap read into tree from persistent strg
	
	start_of_inode_region=sizeof(int)+sizeof(int)*N_DATA_BLOCKS;
	start_of_data_region=sizeof(int)*sizeof(int)*N_DATA_BLOCKS+MAX_INODES*sizeof(inode);

	if(n_nodes==0)//if there are no other nodes than root set up the file system using mkfs
		mkfs();
	else
	{
		root=allocate();//allocate space for inode in tree
		fread(root,sizeof(struct inode),1,disk);//read inode data of root from disk
		read_from_file(root);//from this inode create inodes and read data into them from persi		
	}
	
	fclose(disk);
	return fuse_main( argc, argv, &ops, NULL );
}