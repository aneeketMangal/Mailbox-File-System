/**
 * Simple & Stupid Filesystem.
 * 
 * Mohammed Q. Hussain - http://www.maastaar.net
 *
 * This is an example of using FUSE to build a simple filesystem. It is a part of a tutorial in MQH Blog with the title "Writing a Simple Filesystem Using FUSE in C": http://www.maastaar.net/fuse/linux/filesystem/c/2016/05/21/writing-a-simple-filesystem-using-fuse/
 *
 * License: GNU GPL
 */
#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define BB_DATA "/home/aneeket/test/"
 FILE *out_file;

 
char dir_list[ 256 ][ 256 ];
int curr_dir_idx = -1;

char files_list[ 256 ][ 256 ];
int curr_file_idx = -1;

char files_content[ 256 ][ 256 ];
int curr_file_content_idx = -1;

#include <curl/curl.h>
CURL *curl;
CURLcode res;

struct MemoryStruct {
  char *memory;
  size_t size;
};
struct MemoryStruct chunk;
 
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;
 
  char *ptr = realloc(mem->memory, mem->size + realsize + 1);
  if(!ptr) {
    /* out of memory! */
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }
 
  mem->memory = ptr;
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;
 
  return realsize;
}
 
void add_dir( const char *dir_name )
{
	curr_dir_idx++;
	strcpy( dir_list[ curr_dir_idx ], dir_name );
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "CREATE filesystem/curl");
}

int is_dir( const char *path )
{
	path++; // Eliminating "/" in the path
	
	for ( int curr_idx = 0; curr_idx <= curr_dir_idx; curr_idx++ )
		if ( strcmp( path, dir_list[ curr_idx ] ) == 0 )
			return 1;
	
	return 0;
}

void add_file( const char *filename )
{
	curr_file_idx++;
	strcpy( files_list[ curr_file_idx ], filename );
	
	curr_file_content_idx++;
	strcpy( files_content[ curr_file_content_idx ], "" );
}

int is_file( const char *path )
{
	path++; // Eliminating "/" in the path
	
	for ( int curr_idx = 0; curr_idx <= curr_file_idx; curr_idx++ )
		if ( strcmp( path, files_list[ curr_idx ] ) == 0 )
			return 1;
	
	return 0;
}

int get_file_index( const char *path )
{
	path++; // Eliminating "/" in the path
	
	for ( int curr_idx = 0; curr_idx <= curr_file_idx; curr_idx++ )
		if ( strcmp( path, files_list[ curr_idx ] ) == 0 )
			return curr_idx;
	
	return -1;
}

void write_to_file( const char *path, const char *new_content )
{
	int file_idx = get_file_index( path );
	
	if ( file_idx == -1 ) // No such file
		return;
		
	strcpy( files_content[ file_idx ], new_content ); 
}




static int do_getattr( const char *path, struct stat *st, struct fuse_file_info * temp )
{
	printf( "[getattr] Called\n" );
	printf( "\tAttributes of %s requested\n", path );
	
	st->st_uid = getuid(); // The owner of the file/directory is the user who mounted the filesystem
	st->st_gid = getgid(); // The group of the file/directory is the same as the group of the user who mounted the filesystem
	st->st_atime = time( NULL ); // The last "a"ccess of the file/directory is right now
	st->st_mtime = time( NULL ); // The last "m"odification of the file/directory is right now
	
	if ( strcmp( path, "/" ) == 0 )
	{
		st->st_mode = S_IFDIR | 0777;
		st->st_nlink = 2;
	}
	else if (is_file( path ))
	{
		st->st_mode = S_IFREG | 0777;
		st->st_nlink = 1;
		st->st_size = 1024;
	}
	else{
		return -ENOENT;
	}
	return 0;
}

char* substr(const char *src, int m, int n)
{
    // get the length of the destination string
    int len = n - m;
 
    // allocate (len + 1) chars for destination (+1 for extra null character)
    char *dest = (char*)malloc(sizeof(char) * (len + 1));
 
    // extracts characters between m'th and n'th index from source string
    // and copy them into the destination string
    for (int i = m; i < n && (*(src + i) != '\0'); i++)
    {
        *dest = *(src + i);
        dest++;
    }
 
    // null-terminate the destination string
    *dest = '\0';
 
    // return the destination string
    return dest - len;
}

static int do_readdir( const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_fill_dir_flags temp)
{	
	filler( buffer, ".", NULL, 0, 0); // Current Directory
	filler( buffer, "..", NULL, 0, 0); // Parent Directory
	
	if ( strcmp( path, "/" ) == 0 ) // If the user is trying to show the files/directories of the root directory show the following
	{
		curl_easy_setopt(curl, CURLOPT_URL, "imaps://imap.gmail.com:993/inbox");
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
		curl_easy_perform(curl);
		printf("%s\n", chunk.memory);
		// filler( buffer, "hol", NULL, 0, 0);
	}
	// else{
	// 	filler( buffer, "file.txt", NULL, 0, 0);
	// }
	
	return 0;
}

static int do_read( const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi )
{
	
	int file_idx = get_file_index( path );
	if (file_idx == -1) return -1;
	
	char *content = files_content[ file_idx ];
	memcpy( buffer, content + offset, size );
	return strlen(content) - offset;
}



static int do_mkdir (const char *path, mode_t mode)
{

	path++;
	add_dir( path );
	return 0;
}
static int do_mknod( const char *path, mode_t mode, dev_t rdev )
{
	path++;
	add_file( path );
	
	return 0;
}

static int do_write( const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *info )
{
	write_to_file( path, buffer );
	
	return size;
}

static struct fuse_operations operations = {
	 .getattr	= do_getattr,
    .readdir	= do_readdir,
    .read		= do_read,
    .mkdir		= do_mkdir,
    .mknod		= do_mknod,
    .write		= do_write,
};

int main( int argc, char *argv[] )
{
	chunk.memory = malloc(1); 
    chunk.size = 0;
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_USERNAME, "2019csb1071@gmail.com");
    curl_easy_setopt(curl, CURLOPT_PASSWORD, "assignment5");
    curl_easy_setopt(curl, CURLOPT_URL, "imaps://imap.gmail.com:993/");
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	fuse_main(argc,argv,&operations,NULL);
}