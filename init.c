#include <stdio.h>
#include <stdlib.h>

int main()
{
	FILE* disk=fopen("persistent_st","wb");
	int n_nodes=0;
	//fprintf(disk, "%d",n_nodes);
	fwrite(&n_nodes,sizeof(int),1,disk);
	int* bitmap=(int*)malloc(sizeof(int)*50);
	for(int i=0;i<50;i++)
	{
		bitmap[i]=0;
	}
	fwrite(bitmap,sizeof(int)*50,1,disk);
	fclose(disk);

	// FILE* disk = fopen("persistent_st","rb");
	// int n_nodes;
	// fread(&n_nodes,sizeof(int),1,disk);
	// printf("%d\n",n_nodes);
	// int* bitmap=(int*)malloc(sizeof(int)*50);
	// fread(bitmap,sizeof(int)*50,1,disk);

	// for(int i=0;i<50;i++)
	// {
	// 	printf("%d ",bitmap[i]);
	// }
	// printf("\n");
	// fclose(disk);
}