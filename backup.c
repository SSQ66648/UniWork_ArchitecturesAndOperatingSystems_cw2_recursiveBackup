/******************************************************************************/
/*                                                                            */
/*  CMP-5013A       Architectures and Operating Systems Coursework 2          */
/*                                                                            */
/*  Program:        backup.c                                                  */
/*                                                                            */
/*  Description:    Extends: backupfiles.c                                    */
/*                  if called using 'backup':                                 */
/*                     -Uses -t argument (file/string) as earliest limit to   */
/*                      back up files with a more recent last modified time.  */
/*                      If no time is specified the default of                */
/*                      '1970-01-01 00:00:00' will be used.                   */
/*                     -Uses -f argument as the name of the archive file to be*/
/*                      created. Filename must be provided.                   */
/*                     -Directory containing files to be added to archive can */
/*                      be specified in the final terminal argument.          */
/*                      (If no directory is requested, CWD) will be used.     */
/*                                                                            */
/*                  if called using 'restore'                                 */
/*                      Files in archive-file specified with -f argument will */
/*                      be restored to a created directory (same name as      */
/*                      archive file) found in CWD.                           */
/*                                                                            */
/*  Author:         100166648 / ssq16shu                                      */
/*                                                                            */
/*  Final version:  19/12/2018                                                */
/*                                                                            */
/******************************************************************************/
/*                                                                            */
/*  Notes:  Some print methods used for debugging have been left in the code  */
/*          intentionally as these proved invaluable during debugging and it  */
/*          is useful and user-friendly to see what the program is doing      */
/*          /has done during execution.                                       */
/*                                                                            */
/*          As not a priority, and the code has become complex enough, all    */
/*          'legacy' functionality has been abandoned. This version of the    */
/*          program no longer simply lists the contents of a directory as in  */
/*          tasks 1 & 2.                                                      */
/*                                                                            */
/*          Comment blocks have been reformatted using spaces instead of tabs */
/*          for cross-editor legibility.                                      */
/*                                                                            */
/*          Program now conform to 80 character line                          */
/*                                                                            */
/*          Currently cannot backup cwd. seems to have problems backing up    */
/*          itself while running. No longer possible.                         */
/*                                                                            */
/*          Hard-coded backup directory was hastily changed to a relative     */
/*          directory based on initial CWD before submission so there was no  */
/*          time to exhaustively test the program after this change.          */
/*                                                                            */
/******************************************************************************/



/******************************************************************************/
/*                                                                            */
/*                                Header Files                                */
/*                                                                            */
/******************************************************************************/
#include<ctype.h>           /*char type checking*/
#include<dirent.h>          /*directory entry format*/
#include<fcntl.h>           /*file control*/
#include<grp.h>             /*get string of group id*/
#include<limits.h>          /*variable properties*/
#include<math.h>            /*math functions*/
#include<pwd.h>             /*get string version of user id*/
#include<stdio.h>           /*standard I/O*/
#include<stdlib.h>          /*standard library*/
#include<string.h>          /*string definitions*/
#include<sys/stat.h>        /*stat data structures*/
#include<sys/sysmacros.h>   /*get macro use*/
#include<sys/types.h>       /*system types*/
#include<time.h>            /*get time functions*/
#include<unistd.h>          /*POSIX access*/
#include<utime.h>           /*changing modified timestamp of file*/



/******************************************************************************/
/*                                                                            */
/*                          Pre-method Declarations                           */
/*                                                                            */
/******************************************************************************/
#define BUFFER_SIZE (1024)

//struct to point at directory entry
    struct dirent *entry;
//holder for user keyboard input (progression confirmation)
    char userIn;
//working directory
    char cwd[BUFFER_SIZE];
//cutoff timestamp as t_time type
    time_t cutoffArg;
//recursion counter
    int nNum =0;
//buffer for directory creation for backup
    char restorePath[BUFFER_SIZE];
//default timestamp if none entered
    char defaultTime[BUFFER_SIZE] = "'1970-01-01 00:00:00'";



/******************************************************************************/
/*                                                                            */
/*  Method name:        setTime                                               */
/*                                                                            */
/*  Passed arguments:   modPath                                               */
/*                          path to file to be modified                       */
/*                      modTime                                               */
/*                          time_t value of desired last modified time        */
/*                                                                            */
/*  Description:        Changes the last modified time of restored file to    */
/*                      that of the original file being restored              */
/*                                                                            */
/******************************************************************************/
void setTime(char* modPath, time_t modTime)
{
    struct utimbuf ubuf;
    ubuf.modtime = modTime;
    time(&ubuf.actime);
    if (utime(modPath, &ubuf) !=0)
        perror("utime() error");
}



/******************************************************************************/
/*                                                                            */
/*  Method name:        stringToTime                                          */
/*                                                                            */
/*  Passed arguments:   cutoff                                                */
/*                          quote delimited string containing time info       */
/*                                                                            */
/*  Description:        Converts date-time contained in string into time_t    */
/*                      format to pass to compare and print method.           */
/*                                                                            */
/*  Returns:            sTime                                                 */
/*                          time_t version of passed time-string              */
/*                                                                            */
/******************************************************************************/
time_t stringToTime(char* cutoff)
{
    /*get 'time object' from cutoff (compare to last modified time attribute)*/
    struct tm *dTime = malloc(sizeof(struct tm));
    sscanf(cutoff, "%4d-%2d-%2d %2d:%2d:%2d", 
        &dTime->tm_year,
        &dTime->tm_mon,
        &dTime->tm_mday,
        &dTime->tm_hour,
        &dTime->tm_min,
        &dTime->tm_sec);
    
    /*compensate for Unix time*/
    dTime->tm_year = dTime->tm_year - 1900;
    dTime->tm_mon = dTime->tm_mon - 1;
    dTime->tm_isdst = -1;

    /*make time_t from struct*/
    time_t sTime = mktime(dTime);
    
    return sTime;
}



/******************************************************************************/
/*                                                                            */
/*  Method name:        copyToArchive                                         */
/*                                                                            */
/*  Passed arguments:   archivePath                                           */
/*                          full path of the archive file                     */
/*                                                                            */
/*                      path                                                  */
/*                          full path of the file to be backed-up             */
/*                      info                                                  */
/*                          atruct to access attributes                       */
/*                                                                            */
/*  Description:        Appends the attributes and contents of file to archive*/
/*                                                                            */
/*  Returns:            n/a                                                   */
/*                                                                            */
/******************************************************************************/
int copyToArchive(char* archivePath, char* path, struct stat info)
{
    /*file pointers: archive file, to-backup file*/
    FILE *archiveF, *copyF;
    char cursor;
    printf("Beginning copy\n");

    /*open archive to copy TO in write mode*/
    archiveF =fopen(archivePath, "a");
    if (archiveF == NULL)
    {
        printf("Error opeing archive %s\n",archivePath);
        return EXIT_FAILURE;
    }
    
    /*open file to copy FROM*/
    copyF =fopen(path, "r");
    if(copyF == NULL)
    {
        printf("Cannot open file %s to copy\n",path);
        return EXIT_FAILURE;
    }

    /*write attribute identifier*/
    fprintf(archiveF, "\n\nXXXX_attribute_info_XXXX\n");
    /*copy attributes 'line-by-line'*/
        //type and permissions
    fprintf(archiveF,"mode_%d\n",info.st_mode);
        //hardlinks
    fprintf(archiveF,"nlink_%d\n",info.st_nlink);
        //owner
    fprintf(archiveF,"uid_%d\n",info.st_uid);
        //group
    fprintf(archiveF,"gid_%d\n",info.st_gid);
        //size
    fprintf(archiveF,"size_%d\n",info.st_size);
        //modified date
    fprintf(archiveF,"modified_%d\n",info.st_mtime);
        //name
    fprintf(archiveF,"name_%s\n",entry->d_name);
    printf("completed attribute copy\n");
    
    /*write content identifier*/
    fprintf(archiveF, "\n\nXXXX_content_of_file_XXXX\n");
    /*copy content of file 'line-byline'*/
    cursor =fgetc(copyF);

    printf("Beginning content copy.....\n");
    while (!feof(copyF))
    {
        fputc(cursor,archiveF);
        cursor = fgetc(copyF);
    }
    /*write content finaliser*/
    fprintf(archiveF, "\n\nXXXX_end_of_content_XXXX\n");

    printf("Completed content copy\n");
    fclose(archiveF);
    fclose(copyF);

    return EXIT_SUCCESS;
}



/******************************************************************************/
/*                                                                            */
/*  Method Name:        printAtt                                              */
/*                                                                            */
/*  Passed arguments:   info                                                  */
/*                          struct containing current file attributes         */
/*                      level                                                 */
/*                          count of current depth from top level directory   */
/*                      cutoff                                                */
/*                          time_t containing date/time to compare file to    */
/*                      path                                                  */
/*                          full path of file to be copied/printed            */
/*                      archivePath                                           */
/*                          path to archive file for copyToArchive() call     */
/*                                                                            */
/*  Description:        Using args passed from recursiveWalk function, get and*/
/*                      compare the last modified date of current file to     */
/*                      cutoff. If file has more recent last modified date,   */
/*                      print attributes to screen, mimicing 'ls -l' command  */
/*                     (with additional indentation to hilight the heirachy of*/
/*                     subdirectories and their contents)                     */
/*                                                                            */
/*              Currently contains call to backup method that could be        */
/*              seperated out along with the attribute retreival              */
/*                                                                            */
/******************************************************************************/
/*                                                                            */
/*  Notes:      difftime()  proved to be impossible to use despite being      */
/*              simple. No matter what values i gave it, the 'difference' was */
/*              always identical, hence my int implementation. Any ideas as to*/
/*              why this would be are appriciated                             */
/*                                                                            */
/*              I would like to isolate the print function from the recursion */
/*              and archiving completely. Time did not allow this change.     */
/*              I had planned to utilise a linked list to gather the prints as*/
/*              individual nodes, this would also allow me to find the maximum*/
/*              widths of each feild of the print (ensuring alignment)        */
/*                                                                            */
/******************************************************************************/
void printAtt(struct stat info, int level, time_t cutoff, char* path, 
    char* prog, char* archivePath)
{
    /*access attributes and assign to variables*/
    mode_t  bits    = info.st_mode;     //type and permissions
    nlink_t links   = info.st_nlink;    //hard link counter
    uid_t   uid = info.st_uid;      //owner ID
    struct  passwd  *pwd;           //owner struct
        pwd = getpwuid(uid);    //populate with uid
    gid_t   gid = info.st_gid;      //group ID
    struct  group   *grp;           //group struct      
        grp = getgrgid(gid);    //populate with gid
    off_t   bSize   = info.st_size;     //size in bytes
    time_t  modT    = info.st_mtime;    //last modified time
    char timeBuf[256];          //buffer for formatted timestamp
    strftime(timeBuf, sizeof(timeBuf),"%b %d %H:%M", localtime(&info.st_mtime));

    int difference =0;
    difference = modT - cutoff;
    /*if file-cutoff is negative, file is earlier than cutoff*/
    if (difference < 0)
    {
        printf("File is outside scope\n");
        return;
    }
        
    /*if prog is 'backup' call to copy method*/
    if(strcmp(prog,"backup") == 0)
    {
        printf("\nCopying file...\n");
        copyToArchive(archivePath,path,info);
    }
    
    /*print-tab indent based on current folder-depth (0-n)*/    
    for (int i=0;i<level;i++)
    {
        printf("\t");
    }

    /*print type*/
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
/*  Method Name:        getFileEx                                             */
/*                                                                            */
/*  Passed arguments:   argument                                              */
/*                          string to check                                   */
/*                      point                                                 */
/*                          character to search for final apperance of        */
/*                                                                            */
/*  Description:        Checks argument for character and returns chars after */
/*                                                                            */
/*  Returns:            dot+1                                                 */
/*                          characters after final '_'                        */
/*                      or  ""                                                */
/*                          denoting 'no extension found'                     */
/*                                                                            */
/******************************************************************************/
/*                                                                            */
/*  Notes:      Originally used to check for file type.                       */
/*              Became obsolite during code-revision.                         */
/*              (original plan of using 'tar' file as backup was abandoned on */
/*              advice) Method first kept as extra error-checking layer, then */
/*              utilised to return values after underscore in archive-lines   */
/*                                                                            */
/******************************************************************************/
const char* getFileEx(const char* argument, const char point)
{
    /*get characters after final instance of character (point)*/
    const char* dot = strrchr(argument,point);

    /*return empty string if file extension is not found*/
    if(!dot || dot == argument) 
    {   
        return "";
    }
    return dot+1;
}



/******************************************************************************/
/*                                                                            */
/*  Method Name:        recursiveWalk                                         */
/*                                                                            */
/*  Passed arguments:   filename                                              */
/*                          path of directory to begin method in              */
/*                      level                                                 */
/*                          count of current depth from top level directory   */
/*                      cutoff                                                */
/*                          string containing date/time to compare file to    */
/*                      nNum                                                  */
/*                          counter for 'number of files proccessed'          */
/*                      prog                                                  */
/*                          type of program functionality requested           */
/*                      newPath                                               */
/*                          path to archive to backup to                      */
/*                                                                            */
/*  Description:        Recursively traverse directory heirachy of files and  */
/*                      sub-directories. Skips 'hidden' directories           */
/*                      (namely '.' and '..') Increments level count for each */
/*                      sub-directory entered. updates path for each entry    */
/*                                                                            */
/******************************************************************************/
/*                                                                            */
/*  Notes:      Currently hard-coded for bakup via print method calls. If time*/
/*              allowed, I'd like to have isolated this to just the traversal.*/
/*                                                                            */
/******************************************************************************/
int recursiveWalk(const char *filename, int level, time_t cutoff, int nNum, 
    char* prog, char* newPath)
{
    //struct to access attributes
    struct stat info;
    /*dir pointer*/
    DIR *dp;        
    /*buffer for path*/
    char path[BUFFER_SIZE]; 

    /*open and read dir, with error messages on fail*/
    if (!(dp = opendir(filename)))
    {
        printf("Error opening dir  filename\n");    
        return EXIT_FAILURE;
    }
    if (!(entry = readdir(dp)))
    {
        fprintf(stderr, "Error reading directory\n");   
        return EXIT_FAILURE;
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
            printAtt(info, level, cutoff,path,prog,newPath);
            /*call recursion on subdirectory, increase depth level counter*/
            nNum = recursiveWalk(path, level +1, cutoff, nNum, prog, newPath);
        }
        else
        {
            printAtt(info, level,cutoff,path,prog,newPath);
            nNum = nNum + 1;
        }
    }

    /*close dir pointer*/
    closedir(dp);
    printf("\nTotal files processed: %d\n",nNum);
    return nNum;
}



/******************************************************************************/
/*                                                                            */
/*  Method Name:        restoreFiles                                          */
/*                                                                            */
/*  Passed arguments:   archiveName                                           */
/*                          Name of archive file                              */
/*                      archivePath                                           */
/*                          full path to archive file                         */
/*                                                                            */
/*  Description:        Creates directory in CWD with same name of archive    */
/*                      to be restored. Retreives attributes and creates copy */
/*                      of file from archive with same permissions as original*/
/*                      Copies content to file from archive. Sets last        */
/*                      modified time of new copy to that of original.        */
/*                                                                            */
/******************************************************************************/
/*                                                                            */
/*  Notes:      Successful operation per-file prints could be omitted from    */
/*              this, but they are useful to monitor progress                 */
/*                                                                            */
/*              Some attributes recovered from archive are not currently used */
/*              but are retreived in the event that future versions of this   */
/*              code may require them.                                        */
/*                                                                            */
/******************************************************************************/
int restoreFiles(char* archiveName, char* archivePath)
{
    /*path for file to be created at*/
    char createPath[BUFFER_SIZE];
    FILE *restF;

    /*get cwd and attempt to create backup directory with archive file name*/
    if (getcwd(cwd,sizeof(cwd)) ==NULL)
    {
        printf("Error getting cwd for restore\n");
        return EXIT_FAILURE;
    }

    snprintf(restorePath, BUFFER_SIZE, "%s/%s", cwd, archiveName); 
    printf("Checking for existing backup directory: %s in CWD\n",restorePath);
    struct stat dircheck = {0};
    if (stat(restorePath,&dircheck) ==-1)
    {
        printf("Directory not found: Creating.\n");
        mkdir(restorePath,0700);
    }
    else
    {
        printf("\nDirectory already exists\n"
            "Due to risk of data loss, overwrite or append is not permitted\n"
            "Please check arguments\n\t<program -h :displays help\n");
        return EXIT_FAILURE;
    }

    /*open restorefolder directory*/
    DIR *restP;
    if(!(restP=opendir(restorePath)))
    {
        printf("Error opening backup directory");
        return EXIT_FAILURE;
    }
    printf("Restore directory created at: %s\n\n",restorePath);

    FILE *archP;
    archP=fopen(archivePath,"r");
    if(archP == NULL)
    {
        printf("Error opening archive file\n");
        return EXIT_FAILURE;
    }

/*attribute lines to read*/
    //mode
    mode_t restMode;
    //link
    nlink_t restLink;
    //uid
    uid_t restUid;
    //gid
    gid_t restGid;
    //size
    off_t restSize;
    //modified
    time_t restTime;
    //name
    char restName[BUFFER_SIZE];

    /*buffer for line in archive*/
    char line[BUFFER_SIZE];
    /*buffer to find post-underscore in line*/
    char postScore[256];
    /*switches start/stop processing line/content*/
    int readline =0;
    int readcontent =0;
    /*end pointer of content copy*/
    char* endpt;

    printf("\nBeginning file restoration...\n");
    while (fgets(line,sizeof(line), archP))
    {
        /*read in attribute line*/
        if (readline>0 && readline<8)
        {
            strcpy(postScore,getFileEx(line,'_'));
            /*remove trailing next line*/
            strtok(postScore, "\n");
            /*get value of string digits*/
            long lo = strtol(postScore, &endpt, 10);

            /*hand off value depending on line iteration*/
            switch(readline)
            {
                case 1:
                    restMode = lo;
                    break;
                case 6:
                    restTime = lo;
                    break;
                case 7:
                    strcpy(restName,postScore);
                    break;
            }
            readline++;
        }
        else if(readline>7)
        {
            printf("Attributes assigned\n");
            /*reset readline counter-switch*/
            readline =0;

            /*get full path of created file*/
            snprintf(createPath, sizeof(createPath), "%s/%s", restorePath, 
                restName); 

            /*create file with name and permissions from archive*/
            int restOp = open(createPath, O_WRONLY | O_CREAT, restMode);
            if (restOp == -1)
            {
                printf("Error trying to create restored file");
                return EXIT_FAILURE;
            }   
            close(restOp);
        }
        
        /*read in data*/
        if(readcontent ==1)
        {
            /*check for end of content line*/
            if (strcmp(line,"XXXX_end_of_content_XXXX\n")!=0)
            {
                fprintf(restF, "%s", line);
            }
            else
            {
                /*close written file, set modified time*/
                fclose(restF);
                setTime(createPath,restTime);
                puts("File modified-time changed to original\n");
                readcontent = 0;
            }
        }

        /*toggle switches to read in attributes / content*/
        if (strstr(line,"XXXX_attribute_info_XXXX\n"))
        {
            printf("Found attributes\n");
            readline =1;
        }
        if (strstr(line,"XXXX_content_of_file_XXXX\n"))
        {
            printf("Found content!\n");
            readcontent =1;
            /*open/file pointer to created file*/
            restF =fopen(createPath,"w");
            if(restF == NULL)
            {
                printf("Error opening created file for writing\n");
                return EXIT_FAILURE;
            }
        }
    }

    fclose(archP);
    closedir(restP);
    /*List restored files*/
    printf("Files successfully restored:\n");
    recursiveWalk(restorePath,0,stringToTime(defaultTime),0,"list",archivePath);

    return EXIT_SUCCESS;
}



/******************************************************************************/
/*                                                                            */
/*  Method Name:        checkFile                                             */
/*                                                                            */
/*  Passed arguments:   fileArg                                               */
/*                          path of file to check                             */
/*                                                                            */
/*  Description:        checks if passed file-path exists                     */
/*                                                                            */
/*  Returns:            1 if file exists / 0 if not                           */
/*                                                                            */
/******************************************************************************/
int checkFile(const char* fileArg)
{
    struct stat existBuf;
    int exists = stat(fileArg,&existBuf);

    if(exists ==0)
        return 1;
    else
        return 0;
}



/******************************************************************************/
/*                                                                            */
/*  Method Name:        stringError                                           */
/*                                                                            */
/*  Passed arguments:   type                                                  */
/*                          number denoting where error occurred for print    */
/*                                                                            */
/*  Description:        Prints various error messages to screen depending when*/
/*                      called by stringCheck method                          */
/*                      Exists to tidy the code.                              */
/*                      Avoids multiple copies of error message printing      */
/*                                                                            */
/*  Returns:            EXIT_FAILURE                                          */
/*                          (program unsuccessful if this method is called)   */
/*                                                                            */
/******************************************************************************/
int stringError(int type)
{
    printf("\tString entered does not match correct date format\n"
        "\t\tReason: Incorrect ");
    switch(type)
    {
        case 1:
            printf("String-length ");
            break;
        case 2:
            printf("Date-format ");
            break;
        case 3:
            printf("Time-format ");
            break;
        case 4:
            printf("Time-characters ");
            break;
    }
    printf("entered\n\nPlease check arguments."
        "\n\t-h to display help.\n\n"
        "Returning to terminal.\n\n");
    return EXIT_FAILURE;
}



/******************************************************************************/
/*                                                                            */
/*  Method Name:        stringCheck                                           */
/*                                                                            */
/*  Passed arguments:   argIn                                                 */
/*                          post-directory and file check  potential string   */
/*                                                                            */
/*  Description:        Checks for correct date-time formatting of entered    */
/*                      string. Exists to isolate cluttered nested            */
/*                      if-statements. Passes error check number to           */
/*                      stringError for output of reason for format error     */
/*                                                                            */
/*  Returns:            stringFail                                            */
/*                          notification of string-format failure             */
/*                      (No failure: 0   /   Failure: 1)                      */
/*                                                                            */
/******************************************************************************/
int stringCheck(char* argIn)
{
    int stringFail =1;
    if (strlen(argIn) != 19)
    {
        stringError(1);
        return stringFail;
    }
    else
    {
        printf("\t\tString of correct date-format length detected\n");
        if (argIn[4]!='-' && argIn[7]!='-' && argIn[10]!=' ')
        {
            stringError(2);
            return stringFail;
        }
        else
        {
            printf("\t\tCorrect date format detected\n");
            if (argIn[13]!=':' && argIn[16]!=':')
            {
                stringError(3);
                return stringFail;
            }
            else
            {
                printf("\t\tCorrect time format detected\n");
                if (
                !isdigit(argIn[0]) ||
                !isdigit(argIn[1]) ||
                !isdigit(argIn[2]) ||
                !isdigit(argIn[3]) ||
                !isdigit(argIn[5]) ||
                !isdigit(argIn[6]) ||
                !isdigit(argIn[8]) ||
                !isdigit(argIn[9]) ||
                !isdigit(argIn[11]) ||
                !isdigit(argIn[12]) ||
                !isdigit(argIn[14]) ||
                !isdigit(argIn[15]) ||
                !isdigit(argIn[17]) ||
                !isdigit(argIn[18]) )
                {
                    stringError(4);
                    return stringFail;
                }
                else
                {
                    stringFail = 0;
                }
            }
        }
    }
    return stringFail;
}



/******************************************************************************/
/*                                                                            */
/*  Method Name:        changeDir                                             */
/*                                                                            */
/*  Passed arguments:   arg                                                   */
/*                          final command line argument                       */
/*                      cwd                                                   */
/*                          string containing current working path            */
/*                                                                            */
/*  Description:        Changes CWD if a directory in which to begin-search   */
/*                      is provided.                                          */
/*                                                                            */
/*  Returns:            cwd                                                   */
/*                          modified working directory specified              */
/*                                                                            */
/******************************************************************************/
/*                                                                            */
/*  Notes:      This method has been heavily modified during code revision.   */
/*              Though now obsolite, it has been kept as a seperate method    */
/*              outside main() incase further use presents itself later.      */
/*                                                                            */
/******************************************************************************/
char* changeDir(char* arg, char* cwd)
{
        /*switch directory path to supplied arg*/
        strcpy(cwd,arg);
        printf("New working directory to taverse:\n\t%s\n", cwd);
        return cwd;
}



/******************************************************************************/
/*                                                                            */
/*  Method Name:        checkForFile                                          */
/*                                                                            */
/*  Passed arguments:   argument                                              */
/*                          potential file to verify exists                   */
/*                      argPath                                               */
/*                          struct to access file mode_t                      */
/*                      cwd                                                   */
/*                          string containing current working path            */
/*                                                                            */
/*  Description:        Checks if provided argument is a file.                */
/*                      Calls checkFile() to verify its existance before      */
/*                      discerning if a REG file.                             */
/*                      Extracts time_t information if it is.                 */
/*                                                                            */
/*  Returns:        outT                                                      */
/*                      time_t type last modified time                        */
/*                      (value of -1 if file does not exist)                  */
/*                  EXIT_FAILURE                                              */
/*                      if provided file exists but REG cannot be confirmed   */
/*                                                                            */
/******************************************************************************/
time_t checkForFile(char* argument, struct stat argPath)
{
    time_t outT;
    printf("File provided: %s\n",argument);
    int exists = checkFile(argument);
    if (exists)
    {
        char confirmPrint[BUFFER_SIZE];
        stat(argument,&argPath);
        outT    = argPath.st_mtime;
        if(S_ISREG(argPath.st_mode))
        {
            strftime(confirmPrint, sizeof(confirmPrint), "%b %d %H:%M", 
                localtime(&argPath.st_mtime));
        }
        else
        {
            printf("Cannot verify REG file.\n");
            printf("Returning to terminal\n\n");
            return EXIT_FAILURE;
        }
    printf("\tFile Exists.\n");
    printf("Using last modified date of: ");
    printf("%s\n\n",confirmPrint);
    }
    else
    {
        printf("Cannot find file\n");
        outT = (time_t)(-1);
    }
    return outT;
}



/******************************************************************************/
/*                                                                            */
/*  Method Name:        backup                                                */
/*                                                                            */
/*  Passed arguments:   archiveDir                                            */
/*                          path of archive directory                         */
/*                      archivePath                                           */
/*                          path of archive file                              */
/*                      cwd                                                   */
/*                          string containing current working path            */
/*                      prog                                                  */
/*                          current prog type (backup/restore)                */
/*                          -this should be obsolete when isolation complete  */
/*                      level                                                 */
/*                          subdirectory depth counter for recursiveWalk()    */
/*                          -possibly move declaration?                       */
/*                      argPath                                               */
/*                          struct for information access                     */
/*                                                                            */
/******************************************************************************/
/*                                                                            */
/*  Notes:      Currently hard-coded for bakup via print method calls. If time*/
/*              allows, I'd like to isolate this                              */
/*              Feedback on the differences/integration of creating a file via*/
/*              int and via pointer would be appriciated. Currently, archive  */
/*              is created via 'int method' for error checking and closed     */
/*              again as the rest of the code used FILE pointers to           */
/*              manipulate it and I could not find documentation on how these */
/*              two approaches interacted. I opted for caution and re-opend   */
/*              the created archive later.                                    */
/*                                                                            */
/******************************************************************************/
int backup(char* archiveDir, char* archivePath, char* cwd, char* prog, 
    int level, struct stat argPath)
{
    char oldPath[BUFFER_SIZE];
    //get length of archivePath
    size_t len =strlen(archivePath);
    //malloc len-plus-2 (additional char if incremented)
    char* newPath = malloc(len+1+1);

    /*check archive directory valid then close DIRpointer*/
    DIR *ap;
    if (!(ap = opendir(archiveDir)))
    {
        printf("Error opening archive directory\n"
            "Returning to terminal\n\n");
        return EXIT_FAILURE;
    }
    if (!(entry = readdir(ap)))
    {
        fprintf(stderr, "Error reading archive directory\n");
    }
    closedir(ap);

    /*check for existing file*/
    if(checkFile(archivePath))
    {
        printf("Archive already exists with same name.\n"
            "Please enter numeric option to continue:\n"
            "\t1 Overwrite\n"
            "\t2 Incremental backup\n"
            "\t3 Exit program\n");
        int valid =0;
        while (valid != 1)
        {
            scanf("%d", &userIn);
            switch(userIn)
            {
                case 1:
                    /*Overwrite*/
                    printf("Beginning overwrite:\n");
                    unlink(archivePath);
                    valid =1;
                    break;
                case 2:
                    /*Incremental backup*/
                    printf("Beginning increment:\n");
                    valid =1;
                    //copy (for duplicateArchive() ) and append name
                    strcpy(oldPath,archivePath);
                    printf("Original archive path: %s\n",oldPath);
                    strcpy(newPath,archivePath);
                    newPath[len] = '_';
                    newPath[len+1] = '1';
                    printf("Incremented archive path: %s\n",newPath);

                    //check for existing appended file
                    if (checkFile(newPath))
                    {
                        printf("incremented archive already exists.\n"
                            "Please enter the most recent incremental archive "
                            "you wish to increment.\n"
                            "Returning to terminal\n");
                            return EXIT_FAILURE;
                    }
                    /*get original archive's creation date*/
                    cutoffArg = checkForFile(oldPath,argPath);
                    if (cutoffArg == -1)
                    {
                        printf("Error retreiving original archive modification "
                        "time/nExiting program");
                        return EXIT_FAILURE;
                    }
                    break;
                case 3:
                    /*exit program*/
                    valid=1;
                    printf("Program exit. Returning to terminal...\n\n");
                    return EXIT_FAILURE;
                default:
                    printf("Not a valid option.\n"
                        "Please enter a numeric value above\n");
            }
        }
    }
    else
    {
        /*No existing archive, copy to creation path verbatim*/
        strcpy(newPath,archivePath);
    }
printf("nepath: %s\n",newPath);
    /*open archive file (create or overwrite if exist: user r/w permissions)*/
    int archOut = open(newPath, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | 
        S_IWUSR);
    if (archOut == -1)
    {
        printf("Error creating archive file\n"
        "Exiting program\n");
        return EXIT_FAILURE;
    }

    /*file closed again for FILE pointer use*/
    close(archOut);
    printf("completed file creation\n");

    /*do walk: open and copy */
    printf("beginning recursive walk in %s\n\n",cwd);
    int returnedNum =0;
    returnedNum = recursiveWalk(cwd,level,cutoffArg,nNum,prog,newPath);

    close(archOut);
    return EXIT_SUCCESS;
}



/******************************************************************************/
/*                                                                            */
/*  Method Name:        main                                                  */
/*                                                                            */
/*  Description:        Contains main body of code and calls to functions     */
/*                                                                            */
/******************************************************************************/
/*                                                                            */
/*  Notes:      Feedback regarding a better way to check for options / better */
/*              handling of variables (passed/global etc) would be appriciated*/
/*                                                                            */
/******************************************************************************/
int main(int argc, char *argv[])
{
    /*initial variable declarations*/
//string version of argv[2]
char argIn[BUFFER_SIZE];
    snprintf(argIn, sizeof(argIn),"%s",argv[2]);
//string version of cutoff date
char cutoff[BUFFER_SIZE];
//set subdirectory-depth counter to 0
int level =0;   
//struct to extract full path in multiple methods
struct stat argPath;
//seperate struct to check for directory (for clarity)
struct stat argDir;
    stat(argv[2],&argDir);
//for passing the specific argv to check to 'universal' methods
char argument[BUFFER_SIZE];
//contains the name for backup file to be created
char backupName[BUFFER_SIZE];
//path for archive storage directory
char archiveDir[BUFFER_SIZE];
//original hard-coded archive directory
    //    strcpy(archiveDir,"/nfs/pihome/ssq16shu/backuptesting/backupArchive");
//program type
char prog[BUFFER_SIZE];
//archive
char archivePath[BUFFER_SIZE];



    /*check program function requested (print to confirm)*/
    strcpy(prog,argv[0]);
    printf("\nYou have selected %s functionality:",prog);

    /*assign and check CWD has content otherwise display error message, exit*/
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        printf("\nCWD is %s\n\n", cwd);
    }
    else
    {
        printf("\ngetCWD error");
        return EXIT_FAILURE;
    }

    /*get archive directory from CWD*/
    snprintf(archiveDir,sizeof(archiveDir),"%s/backuparchive", cwd);

    /*confirm and change cwd if specified directory entered (backup only)*/
    int changeCWD;
    if(strcmp(prog,"backup")==0)
    {
        /*create backup archive if not exists */
        char backupCWD[BUFFER_SIZE];
        if (getcwd(backupCWD,sizeof(backupCWD)) ==NULL)
        {
            printf("Error getting cwd for backup\n");
            return EXIT_FAILURE;
        }
        printf("Checking for existing backup directory: %s in CWD\n",archiveDir);
        struct stat dircheck = {0};
        if (stat(archiveDir,&dircheck) ==-1)
        {
            printf("Directory not found: Creating.\n");
            mkdir(archiveDir,0700);
        }
        else
        {
            printf("\nBackup directory exists\n");
        }
        
        printf("Checking for change of directory request...\n");
        stat(argv[argc-1], &argPath);
        if (S_ISDIR(argPath.st_mode))
        {
            printf("\tDirectory found...\n");
            strcpy(cwd,changeDir(argv[argc-1],cwd));
            if (cwd==NULL)
            {
                printf("Error changing cwd.\nReturning to terminal\n\n");
                return EXIT_FAILURE;
            }
        }
        else
        {
            changeCWD =1;
        }
    }
    
    /*command line option switch*/
    int sw;
    /*Check for entered options*/
    int sumOpt;
    int tOpt = 0;
    int fOpt = 0;
    while ((sw = getopt (argc, argv,"ht:f:")) != -1)
        switch (sw)
        {
            case 'h':
                printf("\n------------------ Help: ------------------\n"
                    "This program either:\n"
                    "1: Backs up files from a specified directory to an archive"
                    " file.\n"
                    "\t(with modified-time more recent than supplied cutoff)\n"
                    "\tCut off time is supplied in either:\n"
                    "\t\t-A path to a file (the cutoff being the file modified "
                    "date)\n"
                    "\t\t-Supplied in quote-delimited string format.\n\n"
                    "2: Restores files from archive file to a corresponding "
                    "directory created in CWD\n"
                    "\n----------------- Arguments: ----------------\n"
                    "Backup argument usage:\n"
                    "\tbackup -t <optional-cutoff> -f <name-of-archive> "
                    "<directory-to-back-up>\n"
                    "Restore argument usage:\n"
                    "\trestore -f <archive-to-restore>\n"
                    "\n----------------- Options: ----------------\n"
                    "Optionals are not order-specific, "
                    "but must be immidiately followed by their arguments:\n\n"
                    "\t-t {<filename>,<date>}\n\t\t"
                    "Specify the cut off date with named file:\n"
                    "\t\t(uses last modified date) or date in delimited string "
                    "format\n\t\tdate-string format must conform to "
                    "(including quotes):\n\t\t\t'YYYY-MM-DD HH:MM:SS'\n"
                    "\t\tIf no date/file is supplied, default of Jan 1970 will "
                    "be used\n"
                    "\t-f <filename>\n"
                    "\t\tSpecifies the name of archive to backup to or restore "
                    "from\n\t\tIf no date/file is supplied, default of Jan 1970"
                    "will be used\n"
                    "\t-h\n\t\t Prints this help message\n"
                    "\n---------- Note: ----------\n"
                    "backup requires a directory external to CWD\n\n");
                return EXIT_SUCCESS;

            case 't':
                tOpt = 1;
                /*check if arg has supplied dir location*/
                printf("Checking -t argument...\n");
                stat(optarg,&argDir);
                if (S_ISDIR(argDir.st_mode))
                {
                    /*arg 2 IS directory, assign as directory to begin scan*/
                    strcpy(cwd, optarg);
                    printf("no date/file provided. Using Default:\n");
                    printf("%s\n",defaultTime);
                    break;
                }

                /*check if argument is a file*/
                printf("\tArgument is not a directory, checking for file:\n");
                strcpy(argument,optarg);
                /*check file extension has been found*/
                if(strcmp(getFileEx(argument,'.'),"")!=0)
                {       
                    /*Confirm file exists and get last modified time*/
                    cutoffArg = checkForFile(argument, argPath);
                    if (cutoffArg != -1)
                    {
                        break;
                    }
                    else
                    {
                        printf("Error getting time info from file\n");
                        return EXIT_FAILURE;
                    }
                }
                printf("\tNo file extension found\n\n");
                
                /*check for string-format date time*/
                printf("\tChecking for single-quote delimited string:\n");
                if(stringCheck(argIn)==1)
                {
                    return EXIT_FAILURE;
                }
                printf("\tCorrect digit format detected\n\n");
                printf("\tDate entered: %s\n\n",argIn);
                cutoffArg = stringToTime(argIn);
                break;

            case 'f':
                fOpt = 1;
                printf("Checking -f argument...\n");
                printf("filename provided: %s\n",optarg);
                strcpy(backupName,getFileEx(optarg,'/'));
                printf("Name of archive : %s\n",backupName);
printf("archivepath: %s\n",archivePath);
                /*Complete archive file path*/  
                snprintf(archivePath, BUFFER_SIZE, "%s/%s", archiveDir, 
                    backupName); 
                printf("Full archive path: %s\n",archivePath);
                break;

            default:    /* '?' */
                printf("unknown option flag(s): reverting to default switch\n");
                printf("Please check formatting of arguments\n");
                printf("'backupfiles -h' displays help\n\n");
                return EXIT_FAILURE;
        }

    /*check for entered options (or lack thereof)*/
    sumOpt = tOpt + fOpt;
    if(sumOpt == 0)
    {
        printf("No options entered.\n"
            "Option(s) are required to backup or restore files.\n"
            "\t<program> -h\t:displays help\n\n");
            return EXIT_FAILURE;
    }

    /*begin backup or restore functions*/
    if (strcmp(argv[0], "backup")==0)
    {
        if (changeCWD ==1)
        {
            printf("\tWARNING:\nNo directory change requested.\n"
                "Attempting to back up CWD results in undefined behaviours\n"
                "Please specify an external directory to back up\n"
                "\t<program> -h :displays help\n"
                "Exiting program\n\n");
            return EXIT_FAILURE;
        }
        if(fOpt == 0)
        {
            printf("No destination archive name entered\n"
                "Please check arguments.\n\t <program> -h :displays help\n");
                return EXIT_FAILURE;
        }
        
        backup(archiveDir,archivePath,cwd,prog,level,argPath);
    }
     else if (strcmp(argv[0], "restore")==0)
    {
printf("archive path: %s\n",archivePath);
        if(restoreFiles(backupName,archivePath)==-1)
        {
            printf("Error during restore: Terminating program\n\n");
            return EXIT_FAILURE;
        }
    }

    printf("Completion of program\n"
        "Returning to terminal\n\n");
    return EXIT_SUCCESS;
}
