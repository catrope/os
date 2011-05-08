#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_BLOCK 256

struct file_size
{
	char file[1024];
	off_t size;
};

struct file_size * files;
unsigned int nFiles;
unsigned int maxFiles;

void examineDir(char * name)
{
	DIR *dp;
	struct dirent *de;
	struct stat bufStat;
	char fname[1024];
	
	dp = opendir(name);
	while (de = readdir(dp))
	{
		sprintf(fname, "%s/%s", name, de->d_name);
		
		// ignore directories and hidden files
		if (de->d_type == DT_DIR && de->d_name[0] != '.')
			// recurse to subdirectories
			examineDir(fname);
		else if (de->d_type == DT_REG)
		{
			// if necessary, allocate more space
			if (nFiles >= maxFiles)
			{
				maxFiles += DEFAULT_BLOCK;
				files = realloc(files, maxFiles * sizeof(struct file_size));
			}
			
			// store file name and size
			stat(fname, &bufStat);
			strcpy(files[nFiles].file, fname);
			files[nFiles++].size = bufStat.st_size;
		}
	}
}

int compareSizes(const void * a, const void * b)
{
	return ((struct file_size *)a)->size > ((struct file_size *)b)->size;
}

void compareTwoFiles(char * a, char * b)
{
	FILE *fa, *fb;
	char c;
	fa = fopen(a, "r");
	fb = fopen(b, "r");
	do {
		// keep reading until a character differs
		c = getc(fa);
		if (c != getc(fb)) return;
	} while (c != EOF);
	
	// if we got to the end of one of the first file they are identical
	// assumption: sizes are equal
	printf("%s and %s are identical\n", a, b);
}

void compareFiles(int f)
{
	int i;
	off_t fsize = files[f].size;
	// walk down until the file size differs
	// this makes sure every two files of the same size are compared exactly once
	for(i=f+1; i<nFiles && files[i].size == fsize; i++)
		compareTwoFiles(files[f].file, files[i].file);
}

int main(int argc, char **argv)
{
	int i;
	// allocate initial array
	maxFiles = DEFAULT_BLOCK;
	nFiles = 0;
	files = malloc(maxFiles * sizeof(struct file_size));
	
	// examine the current directory
	examineDir(".");
	
	// sort files by size
	qsort(files, nFiles, sizeof(struct file_size), compareSizes);
	
	// compare every file with others
	for(i=0; i<nFiles; i++) compareFiles(i);
	
	free(files);
	return EXIT_SUCCESS;
}