//
// ffdb - Flat File Database for Embedded devices
//
// (c)2009 Yoics Inc.
//
// mike@yoics
//


#if defined(LINUX)

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/file.h>
#include <ctype.h>

// this are defaults, you can override from command line
#define DEFAULT_DB_FILE    "/data/cfg/config.db"
#define DEFAULT_TMP_FILE   "/data/cfg/ffdb.tmp"
#define DEFAULT_LOCK_FILE  "/tmp/ffdblock.tmp"

#endif

#if defined(WIN32)
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <ctype.h>

#define DEFAULT_DB_FILE    "c:/system.db"
#define DEFAULT_TMP_FILE   "c:/ffdb.tmp"
#define DEFAULT_LOCK_FILE   "c:/lock.tmp"

#endif

// Note that if you need large datatypes, you have to increse line and value size
#define MAX_FILE_SIZE		1024
#define MAX_LINE_SIZE		4096
#define	MAX_VALUE_LEN		2048

#define VERSION				"0.4"

typedef struct search_element_
{
	struct search_element_		*next;
	char						*search_string;
}SEARCH;



// Globals

int verbose=0;
int	xml=0;
int all=0;
int multi=0;
int	quote=1;						// Quotes on by defaults for string output
int force_string=0;                 // Force string output if set
char	user_db[MAX_FILE_SIZE];
char	user_lock[MAX_FILE_SIZE];
char	user_tmp[MAX_FILE_SIZE];



void program_info(void)
{
	printf("FlatFileDataBase (c)2010-12 Yoics (www.yoics.com)\n");
	printf("  Version " VERSION " built " __DATE__ " at " __TIME__ "\n\n");
}

void usage(int argc, char **argv)
{
  program_info();
  printf("usage: %s [-h] [-v] [-d database] [-t temp_file] [-l lock_file] key [value]\n",argv[0]);
  printf("    where -h prints this message.\n");
  printf("          -x output xml\n");
  printf("          -s force string write (not auto detect).\n");
  printf("          -v set verbose mode.\n");
  printf("          -m multiple keys (read only).\n");
  printf("          -a return all keys\n");
  printf("          -q remove quotes on output.\n");
  printf("          -d sets database file [default "DEFAULT_DB_FILE "]\n");
  printf("          -t sets temporary file [default "DEFAULT_TMP_FILE"]\n");
  printf("          -l sets lock file [default "DEFAULT_LOCK_FILE"]\n");
  printf("\n");
}


// Trim the beginning and end of string
void trim(char * s) 
{
    char * p = s;
    int l = strlen(p);

    while(isspace(p[l - 1])) p[--l] = 0;
    while(* p && isspace(* p)) ++p, --l;

    memmove(s, p, l + 1);
}

//
//
//
int
key_match_list(SEARCH *search_list, char *key)
{
	int ret=0,i;
	SEARCH *s=search_list;

	if((0==key) || (0==search_list)) return -1;

	// compair key
	while(s)
	{
		if(verbose) printf("compair database key %s (len %d) to search_list item %s (len %d)\n",key,strlen(key),s->search_string,strlen(s->search_string)); 
		i=0;
		while(s->search_string[i] && key[i])
		{
			// No match
			if (s->search_string[i]!=key[i])
			{
				// Check for wildcard
				if('*'==s->search_string[i])
					ret=1;
				break;
			}
			i++;
		}
		// Match if we are = here
		if(s->search_string[i] == key[i])
			ret=1;

		// Done if match
		if(ret) break;

		// Check next search
		s=s->next;
	}
	return(ret);
}


//
// Free the search list
//
void
free_search_list(SEARCH *search_list)
{
	SEARCH *tmp,*tsearch;

	tsearch=search_list;

	while(tsearch)
	{
		tmp=tsearch->next;
		if(tsearch->search_string) free(tsearch->search_string);
		free(tsearch);
		tsearch=tmp;
	}
}


int
file_lock(FILE *fd)
{
	int	ret=0;
#if defined(WIN32)
	if(verbose) printf("Win32 does not lock for now.\n");
#else
	if(verbose) printf("Lock lockfile\n");
		// We need to wait here +++
#if defined(NB_LOCK_FILE)
		
	while(-1==flock(fileno(fd),LOCK_EX|LOCK_NB))
	{
		perror("lock");
		sleep(1);
	}

#else
	if(-1==flock(fileno(fd),LOCK_EX))
	{
		perror("flock failed\n");
		if(verbose) printf("Lock Failed!\n");
		ret=-1;
	}
	else
	{
		if(verbose) printf("Locked!\n");
	}
#endif
#endif
	return(ret);
}

//
// Open and lock temporary file
//
FILE *get_lock(char *lock_file)
{
	FILE *fd;

	// Make a temporary file
	fd=fopen(lock_file,"w");
	if(fd)
	{
		if(-1==file_lock(fd))
		{
			fclose(fd);
			fd=0;
		}
	}
	return(fd);
}



int
remove_lock(FILE *fd)
{
	if(fd)
	{
#if defined(WIN32)
#else
		if(verbose) printf("Unlock Lockfile\n");
		flock(fileno(fd), LOCK_UN);
		if(-1==flock(fileno(fd),LOCK_UN))
		{
			perror("flock failed to unlock\n");
			if(verbose) printf("UnLock Failed!\n");
		}
		else
		{
			if(verbose) printf("UnLocked!\n");
		}

#endif	
		fclose(fd);

		fd=0;
	}
	else
		return(-1);
	return(0);
}


int isnumeric(const char *str)
{
  while(*str)
  {
    if(!isdigit(*str))
      return 0;
    str++;
  }

  return 1;
}

/*
int isNumber(const char str[])
{
	char *ok;
	int k;

	strtod(str,&ok);

	k = ( (!isspace(*str) ) && (0==strlen(ok) ) );	
	return(k);
}
*/


//
//
//
int
readln_from_a_file(FILE *fp, char *line, int size)
{
	char *p;

    do
	{
      p = fgets( line, size, fp );
	  if(p) trim(line);
	} 
	while( ( p != NULL ) && (( *line == '#') || (*line== '-')) );

    if( p == NULL )
		return( 0 );

    if (strchr(line, '\n'))
          *strchr(line, '\n') = '\0';
    if (strchr(line, '\r'))
          *strchr(line, '\r') = '\0';
    return( 1 );
}

int
readln_from_a_file_clean(FILE *fp, char *line, int size)
{
	char *p;

    p = fgets( line, size, fp );

    if( p == NULL )
		return( 0 );

    if (strchr(line, '\n'))
          *strchr(line, '\n') = '\0';
    if (strchr(line, '\r'))
          *strchr(line, '\r') = '\0';
    return( 1 );
}




int
read_list(char *file_name, SEARCH *search_list)
{
	FILE	*fp;
	char	*subst;
	int		ret=0;
	char	line[MAX_LINE_SIZE];
	char	*tline;
	char	key[MAX_LINE_SIZE];
    char    *ptr;

	if(xml) printf("<database>\n");

	// Read from file
	if(NULL == (fp = fopen( (char *) file_name, "r")) )
	{
		if (verbose) printf("read_db: cannot open database file %s\n",file_name);
		ret=-1;
		if(xml) printf("<res ul=\"0\"/>\n");
	}
	else
	{

		if(xml)
		{
			printf("<keys>\n");
		}

		if (verbose) printf("read_list:  file %s open.\n",file_name);
		// File open search for key
		while(readln_from_a_file(fp, (char *) line, MAX_LINE_SIZE-4))
		{
			// trim whitespace from front
			tline=line;
			while(' '==*tline)
				tline++;
			
			// Zero len line
			if(strlen((char *) tline)==0)
				continue;

            if (verbose) printf("read_db:  line %s\n",line);

			// Search for = only
			subst=(char *) strtok((char *) line,"=");
			
            if((subst==0) || (strlen( (char *) subst)==0))
			{
				// do nothing
			}
			else 
			{
				//copy string and trim
				strcpy(key,subst);
				trim(key);

				if (verbose) printf("read_list:  found key %s (len %d)\n",key,strlen(key));
				
				if(key_match_list(search_list,key))
				{					
					// Key matcch get the rest of the line
					// Get the value
					subst= (char *) strtok(NULL,"\0\n");
					if(subst)
					{
						if (verbose) printf("read_list: value %s\n",subst);
						// We have rest of the line, trim off the leading spaces
						while((' '== *subst) || ('='==*subst))
							subst++;

					
						if(xml)
						{
							if('"'==*subst)
								printf("<%s s=%s/>\n",key,subst);
							else
								printf("<%s ul=\"%s\"/>\n",key,subst);
						}
						else
						{
							printf("%s=%s\n",key,subst);
						}

					}
					else
					{
						// Null string
						if (verbose) printf("read_db:  database key %s exists, value null %s\n",key,subst);
					}
					ret=1;
					if (verbose) printf("read_db:  found key %s with value %s\n",key,subst);
				}
			}
		}
		if ((verbose) && (0==ret)) printf("read_db: key %s not found in file %s\n",key,file_name);
		fclose(fp);
		if(xml) printf("</keys>\n");
	}


	if(xml) printf("</database>\n");
	return ret;
}




int
dump_all_keys(char *file_name)
{
	SEARCH	search_list;
	char	t_search_string[2];

	// create search list with all strings
	strcpy(t_search_string,"*");
	search_list.next=0;
	search_list.search_string=t_search_string;
	return(read_list(file_name, &search_list));
}


/*
If you're opening a file for read:
  1. Open the file THEN
  2. request the non-exclusive lock

*/


/*
//
// ret <0 error
// ret=0 key not found
// ret=1 key found return
//
int
dump_all_keys(char *file_name)
{
	FILE	*fp;
	char	*subst;
	int		ret=0;
	char	line[MAX_LINE_SIZE];
	char	*tline;
	char	key[MAX_LINE_SIZE];
    char    *ptr;

	if(xml) printf("<database>\n");

	// Read from file
	if(NULL == (fp = fopen( (char *) file_name, "r")) )
	{
		if (verbose) printf("read_db: cannot open database file %s\n",file_name);
		ret=-1;
		if(xml) printf("<res ul=\"0\"/>\n");
	}
	else
	{

		if(xml)
		{
			printf("<all>\n");
		}

		if (verbose) printf("read_db:  file %s open.\n",file_name);
		// File open search for key
		while(readln_from_a_file(fp, (char *) line, MAX_LINE_SIZE-4))
		{
			// trim whitespace from front
			tline=line;
			while(' '==*tline)
				tline++;
			
			// Zero len line
			if(strlen((char *) tline)==0)
				continue;

            if (verbose) printf("read_db:  line %s\n",line);

			subst=(char *) strtok((char *) line," =\n");
			
            if((subst==0) || (strlen( (char *) subst)==0))
			{
				// do nothing
			}
			else 
			{
				// Key Found, get the rest of the line
                if (verbose) printf("read_db:  key found %s\n",subst);
				strcpy(key,subst);

				subst= (char *) strtok(NULL,"\0\n");
				if(subst)
				{
                    if (verbose) printf("read_db: set %s\n",subst);
                    // We have rest of the line, trim off the leading spaces
					while((' '== *subst) || ('='==*subst))
						subst++;

					
					if(xml)
					{
						if('"'==*subst)
							printf("<%s s=%s/>\n",key,subst);
						else
							printf("<%s ul=\"%s\"/>\n",key,subst);
					}
					else
					{
						printf("%s=%s\n",key,subst);
					}

				}
				else
				{
					// Null string
					if (verbose) printf("read_db:  database key %s exists, value null %s\n",key,subst);
				}
				ret=1;
				if (verbose) printf("read_db:  found key %s with value %s\n",key,subst);
			}
		}
		if ((verbose) && (0==ret)) printf("read_db: key %s not found in file %s\n",key,file_name);
		fclose(fp);
		printf("</all>\n");
	}


	if(xml) printf("</database>\n");
	return ret;
}
*/


/*
If you're opening a file for read:
  1. Open the file THEN
  2. request the non-exclusive lock

If you're opening a file for write (append):
  1. Open the file THEN
  2. request the exclusive lock THEN
  3. When the lock is obtained, use seek / fseek to reposition
   to the end
and when you come to release the lock:
  1. Ensure all data is written (by closing the file or doing
   a seek) THEN
  2. Release the lock
*/

//
// ret <0 error
// ret=0 key not found
// ret=1 key found return
//
int
read_db(char *file_name, char *key, char *value, int max_val_len)
{
	FILE	*fp;
	char	*subst;
	int		ret=0;
	char	line[MAX_LINE_SIZE];
    char    *ptr;

	// Read from file
	if(NULL == (fp = fopen( (char *) file_name, "r")) )
	{
		if (verbose) printf("read_db: cannot open database file %s\n",file_name);
		ret=-1;
	}
	else
	{

		if (verbose) printf("read_db:  file %s open.\n",file_name);
		// File open search for key
		while(readln_from_a_file(fp, (char *) line, MAX_LINE_SIZE-4))
		{
			if(strlen((char *) line)==0)
				continue;

            if (verbose) printf("read_db:  line %s\n",line);

			subst=(char *) strtok((char *) line," =\n");
			
            if((subst==0) || (strlen( (char *) subst)==0))
			{
				// do nothing
			}
			else if(0==strncmp( (char *) subst,key,MAX_LINE_SIZE-6))
			{
				// Key Found, get the rest of the line
                if (verbose) printf("read_db:  key found %s\n",subst);
				//subst=&line[strlen(subst)+1];
				subst= (char *) strtok(NULL,"\0\n");
				if(subst)
				{
                    if (verbose) printf("read_db: set %s\n",subst);
                    // We have rest of the line, trim off the leading spaces
					while((' '== *subst) || ('='==*subst))
						subst++;
					//
					// Trim off the quote if quotes are disabled
					if((0==quote) && ('"'==*subst))
						subst++;

					// Copy over string
					strncpy((char *) value, (char *) subst, max_val_len);
					//
					// Remove last quote if there and quotes turned off
					if(('"'==value[strlen(value)-1]) && (0==quote))
						value[strlen(value)-1]=0;
					
					value[max_val_len-1]=0;
				}
				else
				{
					// Null string
					if (verbose) printf("read_db:  database key %s exists, value null %s\n",key,value);
					strcpy((char *) value,"");
					if (verbose) printf("read_db:  database key %s exists, value null %s\n",key,value);
				}
				ret=1;
				if (verbose) printf("read_db:  found key %s with value %s\n",key,value);
				break;
			}
		}
		if ((verbose) && (0==ret)) printf("read_db: key %s not found in file %s\n",key,file_name);
		fclose(fp);
	}
	return ret;
}

//
// We only need to lock on write, since we unlink an old file, and rename a tmp file
// to the database file current readers will be still reading the unlinked file.
//  
// We do have to worry about 2 writers, so we will lock writes.
//
// Ret=1 key updated
// ret=0 no key found
// ret=-1 error
//
int
write_db(char *db_file_name,char *tmp_file_name, char *lock_file_name, char *key, char *value)
{
	char	*subst;
	int		ret=0;
	char	line[MAX_LINE_SIZE];
	char	tline[MAX_LINE_SIZE];
	FILE	*read_fd,*write_fd,*lock_fd;

	// get lock
	lock_fd=get_lock(lock_file_name);

	// If we got a lock file
	if(lock_fd>0)
	{
		// open original DB file for read
		// Read from file
		if(NULL == (read_fd = fopen( (char *) db_file_name, "r")) )
		{
			if (verbose) printf("write_db: cannot open database file %s.\n",db_file_name);
			ret=-1;
		}
		else
		{
            if (verbose) printf("write_db: opened database file %s.\n",db_file_name);
			// Open temp file
			if(NULL == (write_fd = fopen( (char *) tmp_file_name, "w")) )
			{
				if (verbose) printf("write_db: cannot open temp file %s.\n",tmp_file_name);
				ret=-1;
				fclose(read_fd);
			}
			else
			{
				// Search through keys, replace the key we like
				while(readln_from_a_file_clean(read_fd, (char *) line, MAX_LINE_SIZE-4))
				{
                    if (verbose) printf("write_db: readline %s.\n",line);
					if(strlen((char *) line)==0)
					{
						fprintf(write_fd,"\n");
						continue;
					}
					// Make backup
					strcpy(tline,line);
					subst=(char *) strtok((char *) tline," =\n");

					if((0==subst) || (strlen( (char *) subst)==0))
					{
						fprintf(write_fd,"\n");
					}
					else if(0==strcmp( (char *) subst,key))
					{
						// Find out if value is number or alphanum
					
						// Key Found, copy over new value
						//
						// Quote it if a string and not already quoted
						// Do not quote if a number
						//
                        if((force_string) || (!isnumeric(value) && (value[0]!='"')))
                        {
                            // Value is a string quote if it is not already quoted
                            fprintf(write_fd,"%s = \"%s\"\n",key,value);
                        }
                        else
						{
							// Value is a number do not quote
							fprintf(write_fd,"%s = %s\n",key,value);
						}
						ret=1;
						if (verbose) printf("write_db:  found key %s, replace with value %s.\n",key,value);
					}
					else
					{
						//Just write the line as is
						fprintf(write_fd,"%s\n",line);
					}
				}
				fclose(read_fd);
				fclose(write_fd);

				if(1==ret) 
				{
					// We've updated the file, lets rename it over the database file

					if(-1==rename(tmp_file_name,db_file_name))
					{
						if (verbose) 
						{
							perror("write_db: faild to update database.\n");
						}
					}
					else
					{
						if (verbose) printf("write_db: database updated.\n");
					}
				}
				else
				{
					if (verbose) printf("write_db: key %s not found, nothing done.\n",key);
				}
			}
		}
		//
		// Finally unlock
		remove_lock(lock_fd);
	}
	else
	{
		if (verbose) printf("write_db: got lock file.\n");
		ret=-1;
	}

	return(ret);
}






int main(int argc, char *argv[])
{
	int			ci,ret=-1;
	char		value[MAX_VALUE_LEN];
	int			count=0;
	SEARCH		*search_list=0,*search_tmp;



	//------------------------------------------------------------------
	// Set default values
	//------------------------------------------------------------------
	strcpy(user_db,DEFAULT_DB_FILE);
	strcpy(user_lock,DEFAULT_LOCK_FILE);
	strcpy(user_tmp, DEFAULT_TMP_FILE);


	//------------------------------------------------------------------
	// Argument Scan command line args, first for -h
	//------------------------------------------------------------------
	for(ci=1;ci<argc;ci++)
	{
		if(0 == strncmp(argv[ci], "-d", 2))
		{
			// Get user specified DB
			ci++;
			if(ci<argc)
			{
				strncpy(user_db,argv[ci],MAX_FILE_SIZE);
				user_db[MAX_FILE_SIZE-1]=0;
			}
		}
		else if(0 == strncmp(argv[ci], "-l", 2))
		{
			// Get user specified DB
			ci++;
			if(ci<argc)
			{
				strncpy(user_lock,argv[ci],MAX_FILE_SIZE);
				user_lock[MAX_FILE_SIZE-1]=0;
			}
		}
		else if(0 == strncmp(argv[ci], "-t", 2))
		{
			// Get user specified DB
			ci++;
			if(ci<argc)
			{
				strncpy(user_tmp,argv[ci],MAX_FILE_SIZE);
				user_tmp[MAX_FILE_SIZE-1]=0;
			}
		}
        else if(0 == strncmp(argv[ci], "-s", 2))
        {
            force_string=1;
        }
		else if(0 == strncmp(argv[ci], "-q", 2))
		{
			// Remove Quotes
			quote=0;
		}
		else if(0 == strncmp(argv[ci], "-x", 2))
		{
			// xml out
			xml=1;
		}
		else if(0 == strncmp(argv[ci], "-a", 2))
		{
			// rreturn all keys
			all=1;
		}
		else if(0 == strncmp(argv[ci], "-m", 2))
		{
			// rmultiple
			multi=1;
		}
		else if(0 == strncmp(argv[ci], "-v", 2))
		{
			// Verbose
			verbose=1;
		}
		else if(0 == strncmp(argv[ci], "-h", 2))
		{
			// print help
			usage(argc,argv);
			exit(1);
		}
		else
		{
			// No more args break
			break;
		}
	}

	if(all)
	{
		if(verbose) printf("Read all keys\n");
		ret=dump_all_keys(user_db);
		if(ret>0)
		{
			if(verbose) printf("read_db failed\n");
		}
		else if (ret<0)
		{
			if(verbose) printf("read_db failed\n");
		}
		else
		{
			if(verbose) printf("read_db returned key not found.\n");
		}
	}
	else if (multi)
	{
		int fail=0;
		// parse command line for multiple search strings
		count=0;
		while(argc>ci+count)
		{
			if(verbose) printf("Read key %s\n",argv[ci+count]);

			search_tmp=malloc(sizeof(SEARCH));
			if(search_tmp)
			{
				// malloc +1 for terminator
				search_tmp->search_string=malloc( strlen(argv[ci+count])+1 );
				if(search_tmp->search_string)
				{
					// copy search string
					strcpy(search_tmp->search_string,argv[ci+count]);
					// Hook it up
					search_tmp->next=search_list;
					search_list=search_tmp;
				}
				else
					break;
			}
			else
				break;
			count++;
		}
		if(argc<=ci+count)
		{
			// Do the read multi
			read_list(user_db,search_list);
		}
		free_search_list(search_list);
	}
	else if(argc==ci+1)
	{
		// Read Key
		if(verbose) printf("Read key %s\n",argv[ci]);
		ret=read_db(user_db, argv[ci], value, MAX_VALUE_LEN);
		if(ret>0)
		{
			if(xml)
			{
				printf("<read>\n");
				printf("	<res ul=\"0\"/>\n");
				printf("	<key>\n");
				if('"'==value[0])
					printf("		<%s s=%s>\n",argv[ci],value);
				else
					printf("		<%s ul=\"%s\">\n",argv[ci],value);					
				printf("	</key>\n");
				printf("</read>\n");
			}
			else
			{
				printf("%s",value);
			}
		}
		else if (ret<0)
		{
			if(verbose) printf("read_db failed\n");
			if(xml) printf("");
		}
		else
		{
			if(verbose) printf("read_db returned key not found.\n");
		}
	}
	else if(argc==ci+2)
	{
		// Write Key
		if(verbose) printf("Write Key %s with value %s\n",argv[ci], argv[ci+1]);
		if(0<write_db(user_db,user_tmp,user_lock, argv[ci], argv[ci+1]))
		{
			if(xml)
			{
				if(verbose) printf("Read key %s\n",argv[ci]);
			}
		}
		else
		{
			if(verbose) printf("failed\n");
		}
	}
	else
	{
		// print help
		ret=-1;
		usage(argc,argv);
	}

	return(ret);
}



