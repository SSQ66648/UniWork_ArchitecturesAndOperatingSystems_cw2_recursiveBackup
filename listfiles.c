/******************************************************************************/
/*                                                                            */
/*  CMP-5013A   Architectures and Operating Systems Coursework 2              */
/*                                                                            */
/*  Program:    listfiles.c                                                   */
/*                                                                            */
/*  Description:    Recursively traverse a directory (CWD)                    */
/*          and any contained subdirecories.                                  */
/*          Display file attributes, mimicking output of the termainal        */
/*          command: "ls -l"                                                  */
/*                                                                            */
/*  Author:     100166648 / ssq16shu                                          */
/*                                                                            */
/*  Final version:  12/12/2018                                                */
/*                                                                            */
/******************************************************************************/
/*                                                                            */
/*  Notes:  Some print methods used for debugging have been left in the       */
/*          code intentionally as these proved invaluable during original     */
/*          troubleshooting, and may prove to be useful again.                */
/*                                                                            */
/*          Code edited to conforme to 80 character lines                     */
/*                                                                            */
/******************************************************************************/



/******************************************************************************/
/*                                                                            */
/*              Header Files                                                  */
/*                                                                            */
/******************************************************************************/
#include<fcntl.h>       /*file control*/
#include<stdlib.h>      /*standard library*/
#include<unistd.h>      /*POSIX access*/
#include<sys/types.h>       /*system types*/
#include<sys/stat.h>        /*stat data structures*/
#include<string.h>      /*string definitions*/
#include<stdio.h>       /*standard I/O*/
#include<dirent.h>      /*directory entry format*/
#include<limits.h>      /*variable properties*/
#include<sys/sysmacros.h>   /*get macro use*/
#include<pwd.h>         /*get string version of user id*/
#include<grp.h>         /*get string of group id*/
#include<time.h>        /*get time functions*/
#include<math.h>        /*math functions*/



/******************************************************************************/
/*                                                                            */
/*              Pre-method definitions                                        */
/*                                                                            */
/******************************************************************************/
#define BUFFER_SIZE (1024)
struct stat info;
struct dirent *entry;



/******************************************************************************/
/*                                                                            */
/*  Method Name:        printAtt                                              */
/*                                                                            */
/*  Passed arguments:   info                                                  */
/*                          struct containing current file attributes         */
/*                      level                                                 */
/*                          count of current depth from top level directory   */
/*                                                                            */
/*  Description:        Using args passed from recursiveWalk function, get and*/
/*                      compare the last modified date of current file to     */
/*                      cutoff. If file has more recent last modified date,   */
/*                      print file attribute information.                     */
/*                      attributes to screen, mimicing 'ls -l' terminal       */
/*                      command (with additional indentation to hilight the   */
/*                      heirachy of subdirectories and their contents         */
/*                                                                            */
/******************************************************************************/
/*                                                                            */
/*  Notes:          If time allowed i wanted to utilise a linked-list to      */
/*                  collect the information "to-print" and have this method   */
/*                  only handle the actual printing. This would allow prior   */
/*                  knowledge of exact feild character widths (for alignment) */
/*                                                                            */
/*  To do:          print correct feild widths (variable size feild)          */
/*  Solution(s):    seperate subfunctions to gather list and to print         */
/*                                                                            */
/******************************************************************************/
void printAtt(struct stat info, int level)
{
    /*access attributes and assign to variables*/
    mode_t  bits    = info.st_mode;     //type and permissions
    nlink_t links   = info.st_nlink;    //hard link counter
    uid_t   uid = info.st_uid;          //owner ID
    struct  passwd  *pwd;               //owner struct
        pwd = getpwuid(uid);            //populate with uid
    gid_t   gid = info.st_gid;          //group ID
    struct  group   *grp;               //group struct      
        grp = getgrgid(gid);            //populate with gid
    off_t   bSize   = info.st_size;     //size in bytes
    time_t  modT    = info.st_mtime;    //last modified time
    char timeBuf[256];                  //buffer for formatted timestamp
    strftime(timeBuf, sizeof(timeBuf), "%b %d %H:%M", 
        localtime(&info.st_mtime));

    /*TO DO: get number of digits in size field*/

    /*print tab indent based on current folder-depth (0-n)*/    
    for (int i=0;i<level;i++)
    {
        printf("\t");
    }

    /*print type and permissions */
    printf( (S_ISDIR(info.st_mode) ) ? "d" : "-");
    /*print permissions*/
    printf( (bits & S_IRUSR) ? "r" : "-");
    printf( (info.st_mode & S_IWUSR) ? "w" : "-");
    printf( (info.st_mode & S_IXUSR) ? "x" : "-");
    printf( (info.st_mode & S_IRGRP) ? "r" : "-");
    printf( (info.st_mode & S_IWGRP) ? "w" : "-");
    printf( (info.st_mode & S_IXGRP) ? "x" : "-");
    printf( (info.st_mode & S_IROTH) ? "r" : "-");
    printf( (info.st_mode & S_IWOTH) ? "w" : "-");
    printf( (info.st_mode & S_IXOTH) ? "x" : "-");

    /*print hardlinks counter*/
    printf(" %zu", links);
    /*print owner*/
    printf(" %s", pwd->pw_name);
    /*print group*/
    printf(" %s", grp->gr_name);
    /*print size(width variable)*/
    printf(" %d", bSize);
    /*print last modified date*/
    printf(" %s", timeBuf);
    /*print name of file*/
    printf(" %s", entry->d_name);
    printf("\n");
}



/******************************************************************************/
/*                                                                            */
/*  Method Name:        recursiveWalk                                         */
/*                                                                            */
/*  Passed arguments:   filename                                              */
/*                          path of directory to begin method in              */
/*                      level                                                 */
/*                          count of current depth from top level directory   */
/*                                                                            */
/*  Description:        Recursively traverse directory heirachy of files and  */
/*                      sub-directories. Skips 'hidden' directories           */
/*                      (namely '.' and '..') Increments level count for each */
/*                      sub-directory entered. updates path for each entry    */
/*                                                                            */
/******************************************************************************/
void recursiveWalk(const char *filename, int level)
{
    /*dir pointer*/
    DIR *dp;        
    /*buffer for path*/
    char path[BUFFER_SIZE]; 

    /*open and read dir, with error messages on fail*/
    if (!(dp = opendir(filename)))
    {
        printf("Error opening dir  filename\n");    
        return;
    }
    if (!(entry = readdir(dp)))
    {
        fprintf(stderr, "Error reading directory\n");   
        return;
    }

    /*while next entry exists*/
    while ((entry = readdir(dp)) != NULL)   
    {

        /*get full entry path*/
        snprintf(path, BUFFER_SIZE, "%s/%s", filename, entry->d_name); 
        if (stat(path, &info) !=0)
            printf("cannot access %s\n", path);     
        /*debugging error output*/
        if (stat(path, &info) == -1)
        {
            perror("while calling stat()\n");
        }

//printf("starting directory check");
        /*find if (sub)directory*/
        if (S_ISDIR(info.st_mode) != 0)
        {
            /*skip 'hidden' paths*/
            if (strcmp(entry->d_name, ".") ==0 || 
                strcmp(entry->d_name, "..") ==0)
            {
                continue;
            }
            /*print attributes*/
            printAtt(info, level);
            /*call recursion on subdirectory, increase depth level counter*/
            recursiveWalk(path, level +1);
        }
        else
        {
            printAtt(info, level);
        }
    }
    /*close dir pointer*/
    closedir(dp);
}



/******************************************************************************/
/*                                                                            */
/*  Method Name:        main                                                  */
/*                                                                            */
/*  Description:        Contains main body of code and calls to functions     */
/*                                                                            */
/******************************************************************************/
int main(int argc, char *argv[])
{
    /*find and display current working directory*/  
    char cwd[BUFFER_SIZE];
    
    /*assign and check CWD has content otherwise display error message and exit*/
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        printf("\nCWD is %s\n", cwd);
    }
    else
    {
        printf("\ngetCWD error");
        return EXIT_FAILURE;
    }

    printf("beginning recursive walk in CWD\n\n");
    /*set depth-level counter to 0*/
    int level =0;

    /*call recursive traversal method*/
    recursiveWalk(cwd, level);

    /*skip lines on final output (formatting)*/
    printf("\n\n");

    return EXIT_SUCCESS;    
}
