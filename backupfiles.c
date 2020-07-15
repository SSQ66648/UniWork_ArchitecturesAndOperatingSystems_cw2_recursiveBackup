/******************************************************************************/
/*                                                                            */
/*  CMP-5013A   Architectures and Operating Systems Coursework 2              */
/*                                                                            */
/*  Program:    backupfiles.c                                                 */
/*                                                                            */
/*  Description:    Extends: listfiles.c                                      */
/*                  (Recursively traverse a directory (cwd)                   */
/*                  and any contained subdirecories.                          */
/*                  Display file attributes, mimicking output of the termainal*/
/*                  command: "ls -l")                                         */
/*                                                                            */
/*          Adds functionality to display details of files newer than time    */
/*          specified by command line argument                                */
/*          (named file's last modified date or delimited string)             */
/*          if no date is specified, use default date of:                     */
/*              1970-01-01 00:00:00                                           */
/*          Final command arg is directory from which to begin file search    */
/*          Includes command line options:                                    */
/*          -t {filename,date}  specify cut off date with string or file      */
/*          -h          print help message to explain cmd use                 */
/*                                                                            */
/*  Author:     100166648 / ssq16shu                                          */
/*                                                                            */
/*  Final version:  15/12/2018                                                */
/*                                                                            */
/******************************************************************************/
/*                                                                            */
/*  Notes:  Some print methods used for debugging have been left in the       */
/*          code intentionally as these proved invaluable during original     */
/*          troubleshooting, and may prove to be useful again.                */
/*          As has the entire functionality of listfiles.c                    */
/*          This has been utilised as 'additional' functionality.             */
/*                                                                            */
/******************************************************************************/



/******************************************************************************/
/*                                                                            */
/*              Header Files                                                  */
/*                                                                            */
/******************************************************************************/
#include<ctype.h>           /*char type checing*/
#include<fcntl.h>           /*file control*/
#include<stdlib.h>          /*standard library*/
#include<unistd.h>          /*POSIX access*/
#include<sys/types.h>       /*system types*/
#include<sys/stat.h>        /*stat data structures*/
#include<string.h>          /*string definitions*/
#include<stdio.h>           /*standard I/O*/
#include<dirent.h>          /*directory entry format*/
#include<limits.h>          /*variable properties*/
#include<sys/sysmacros.h>   /*get macro use*/
#include<pwd.h>             /*get string version of user id*/
#include<grp.h>             /*get string of group id*/
#include<time.h>            /*get time functions*/
#include<math.h>            /*math functions*/



/******************************************************************************/
/*                                                                            */
/*              Pre-method definitions                                        */
/*                                                                            */
/******************************************************************************/
#define BUFFER_SIZE (1024)

struct stat info;
struct dirent *entry;
int level;



/******************************************************************************/
/*                                                                            */
/*  Method name:        stringToTime                                          */
/*                                                                            */
/*  Passed arguments:   cutoff                                                */
/*                          quote delimited string containing time info       */
/*                                                                            */
/*  Description:        Converts date-time contained in string into time_t    */
/*                          format to pass to compare and print method        */
/*                                                                            */
/*  Returns:            time_t version of received time-string                */
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
/*  Method Name:        printAtt                                              */
/*                                                                            */
/*  Passed arguments:   info                                                  */
/*                          struct containing current file attributes         */
/*                      level                                                 */
/*                          count of current depth from top level directory   */
/*                      cutoff                                                */
/*                          time_t containing date/time to compare file to    */
/*                                                                            */
/*  Description:        Using args passed from recursiveWalk function, get and*/
/*                      compare the last modified date of current file to     */
/*                      cutoff. If file has more recent last modified date,   */
/*                      print attributes to screen, mimicing 'ls -l' terminal */
/*                      command (with additional indentation to hilight the   */
/*                      heirachy of subdirectories and their contents         */
/*                                                                            */
/******************************************************************************/
/*                                                                            */
/*  Notes:  This method still needs to be isolated form all other functions   */
/*          other than 'printing'                                             */
/*                                                                            */
/******************************************************************************/
void printAtt(struct stat info, int level, time_t cutoff)
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
    strftime(timeBuf, sizeof(timeBuf), "%b %d %H:%M", localtime(&info.st_mtime));       

    /*compare times*/
    double diff = difftime(modT, cutoff);

    /*exit method if cutoff point is not default or last modified attribute is more recent*/
    if (diff < 0)
        return;

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
/*  Method Name:        recursiveWalk                                         */
/*                                                                            */
/*  Passed arguments:   filename                                              */
/*                          path of directory to begin method in              */
/*                      level                                                 */
/*                          count of current depth from top level directory   */
/*                      cutoff                                                */
/*                          string containing date/time to compare file to    */
/*                                                                            */
/*  Description:        Recursively traverse directory heirachy of files and  */
/*                      sub-directories. Skips 'hidden' directories           */
/*                      (namely '.' and '..') Increments level count for each */
/*                      sub-directory entered. updates path for entry         */
/*                                                                            */
/******************************************************************************/
/*                                                                            */
/*  To do:      Seperate out 'listing' of files to print                      */
/*  Solution(s):    seperate function chain ie walk-list-print                */
/*                                                                            */
/******************************************************************************/
void recursiveWalk(const char *filename, int level, time_t cutoff)
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
            if (strcmp(entry->d_name, ".") ==0 || strcmp(entry->d_name, "..") ==0)
            {
                continue;
            }
            /*print attributes*/
            printAtt(info, level, cutoff);
            /*call recursion on subdirectory, increase depth level counter*/
            recursiveWalk(path, level +1, cutoff);
        }
        else
        {
            printAtt(info, level,cutoff);
        }
    }
    /*close dir pointer*/
    closedir(dp);
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
/*                                                                            */
/*  Notes:  Additional 'if-not-exist:create' functionality could be useful    */
/*          Will consider adding it to task3: backup.c                        */
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
/*                      Exists to tidy code. Avoids multiple copies of 'print'*/
/*                                                                            */
/******************************************************************************/
int stringError(int type)
{
    printf("\tString entered does not match correct date format\n");
    printf("\t\tReason: Incorrect ");
    switch(type)
    {
        case 1:
            printf("\tString-length ");
            break;
        case 2:
            printf("\tDate-format ");
            break;
        case 3:
            printf("Time-format ");
            break;
        case 4:
            printf("Time-characters ");
            break;
    }
    printf("entered\n\n\tPlease check arguments. -h to display help.\n");
    printf("\treturning to terminal.\n");
    return EXIT_FAILURE;
}



/******************************************************************************/
/*                                                                            */
/*  Method Name:        stringCheck                                           */
/*                                                                            */
/*  Passed arguments:   argIn                                                 */
/*                          argv[2]: post-directory and file check            */
/*                                                                            */
/*  Description:        Checks argv[2] for correct date-time formatting of    */
/*                      entered string. Exists to isolate cluttered nested    */
/*                      if-statements from main.                              */
/*                      Passes error check number to stringError for output   */
/*                                                                            */
/******************************************************************************/
int stringCheck(char* argIn)
{
    int stringFail =1;
    if (strlen(argIn) != 19)
    {
        stringError(1);
    }
    else
    {
        printf("\tString of correct date-format length detected\n");
        if (argIn[4]!='-' && argIn[7]!='-' && argIn[10]!=' ')
        {
            stringError(2);
        }
        else
        {
            printf("\tCorrect date format detected\n");
            if (argIn[13]!=':' && argIn[16]!=':')
            {
                stringError(3);
            }
            else
            {
                printf("\tCorrect time format detected\n");
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
/*  Method Name:        legacy                                                */
/*                                                                            */
/*  Passed arguments:   argc                                                  */
/*                          command line argument count                       */
/*                      argv[]                                                */
/*                          command line arguments                            */
/*                      cwd                                                   */
/*                          string containing working directory               */
/*                      cutoff                                                */
/*                          string containing impossible cutoff date          */
/*                     argPath                                                */
/*                          struct to gain attributes of  file or directory   */
/*                                                                            */
/*  Description:        Contains legacy functionality of Task1: list info for */
/*                      all files/subdirectories in CWD.                      */
/*                      Additional function: ability to specify directory     */
/*                      to recursively list all details of.                   */
/*                      Exists to seperate out additional functioonality from */
/*                      main program (comment out method calling to disable)  */
/*                                                                            */
/******************************************************************************/
/*                                                                            */
/*  Notes:      While passing so many argument is unessisary if this remained */
/*              in body of main, the ability to simply disable this section   */
/*              (as not on cw breif /debugging) outweighs the extra  method   */
/*                                                                            */
/******************************************************************************/
int legacy(int argc, char* argv[], char* cwd, char* cutoff, struct stat argPath)
{
    char userIn;
    int exitNum =1;
    /*program bahaiviour based on ammount of arguments*/
    if(argc ==1)
    {
        printf("\nNo arguments or options entered.\n");
        printf("Enter 'y' to continue displaying all file details of CWD:\n");
        printf("Enter 'n' to exit to terminal:\n");
        scanf("%c", &userIn);
        if (userIn=='y')
        {
            /*call recursive traversal and print method*/
            printf("beginning recursive walk in %s\n\n",cwd);
            recursiveWalk(cwd, level, stringToTime(cutoff));
            /*skip lines on final output (formatting)*/
            printf("\n\n");
            return exitNum;
        }
        else if (userIn=='n')
        {
            printf("returning to terminal\n\n");
            return exitNum;
        }
        else
        {
            printf("invalid choice.\nReturning to terminal\n\n");
            return exitNum;
        }
    }
    else if (argc==2)
    {
        /*single argument(not h/t flag): display all for specified directory*/
        /*(only checks for three flags as no other valid 2-arg entry to check 
            -could be refine later)*/
        if (strcmp(argv[1], "-h") !=0 
            && strcmp(argv[1], "-t") != 0)
        {
            printf("\nSingle arg detected.\n");
            printf("Checking if arg is a directory...\n");
            stat(argv[1], &argPath);
            if (S_ISDIR(argPath.st_mode))
            {
                printf("Directory confirmed.\n");
                printf("Display all file info in directory location supplied by"
                    "arg? (y/n):\n");
                scanf("%c",&userIn);
                if (userIn=='y')
                {
                    strcpy(cwd,argv[1]);
                    printf("beginning recursive walk in %s\n\n",cwd);
                    recursiveWalk(cwd, level,stringToTime(cutoff));
                    printf("\n\n");
                    return exitNum;
                }

            }
            printf("Unable to confirm directory. Please check arguments. "
                "-h displays help\n");
            printf("Exiting to terminal.\n\n");
            return exitNum;
        }
        else if (strcmp(argv[1],"-t")==0)
        {
            printf("Flag(s) entered without argument(s).\n");
            printf("Please provide correct input format.\n"
                "\t-h displays help.\n ");
            return exitNum;
        }
        else
        {
            exitNum = 0;
        }
        return exitNum;
    }
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
    /*initial declarations*/    
    char cwd[BUFFER_SIZE];
    char argIn[BUFFER_SIZE];
    char cutoff[BUFFER_SIZE];
        /*set timestring to impossible to prior value (print all)*/
        strcpy(cutoff, "'0000-00-00 00:00:00'");
    char defaultTime[BUFFER_SIZE] = "'1970-01-01 00:00:00'";
    level =0;   
    struct stat argPath;
    struct stat argDir;


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

    /*Legacy functionality: comment out to disable*/
    if(argc == 1 || argc ==2)
    {
        if(legacy(argc,argv,cwd,cutoff,argPath))
            return EXIT_SUCCESS;
    }

    /*command line option switch*/
    int sw;
    opterr = 0;
    while ((sw = getopt (argc, argv,"ht:")) != -1)
        switch (sw)
        {
            case 'h':
                printf("\n------------------ Help: ------------------\n");
                printf("This program lists the details of files modified more "
                    "recently\nthan a supplied file or date supplied in "
                    "quote-delimited string format.\n"
                    "The use of this program's command line arguments are as "
                    "follows:\n"
                    "backupfiles  <option(s)> <directory_to_begin_search>\n"
                    "\n----------------- Options: ----------------\n"
                    "\t-t {<filename>,<date>}\tspecify the cut off date with "
                    "named file\n"
                    "\t\t(uses last modified date) or date in delimited string "
                    "format\n\t\tdate-string format must conform to "
                    "(including quotes):\n\t\t'YYYY-MM-DD HH:MM:SS'\n"
                    "\t\tif no date/file is supplied, default of Jan 1970 will "
                    "be used\n\t-h Prints this help message\n"
                    "\n---------- Legacy Functionality: ----------\n"
                    "\t-No arguments will display details of files in CWD\n"
                    "\t-A single argument of a full directory path will display"
                    "details\n\t of files at that directory\n\n");
                return EXIT_SUCCESS;

            case 't':
                /*take string copy of argv[2]*/
                snprintf(argIn, sizeof(argIn),"%s",argv[2]);    
                if (argc==4)
                {
                    /*confirm and change cwd to specified directory*/
                    stat(argv[3], &argPath);
                    if (S_ISDIR(argPath.st_mode))
                    {
                        /*switch directory path to supplied arg*/
                        strcpy(cwd,argv[3]);
                        printf("Directory switch detected.\n");
                        printf("New working directory to taverse:\n\t%s\n", 
                            cwd);
                    }
                    else
                    {
                        printf("Cannot verify change of directory.\n");
                        printf("Please check arguments. -h displays help\n");
                        return EXIT_FAILURE;
                    }
                }

                /*check if arg has supplied dir location*/
                stat(argv[2],&argDir);
                if (S_ISDIR(argDir.st_mode))
                {
                    /*arg 2 IS directory, assign as directory to begin scan*/
                    strcpy(cwd, argv[2]);
                    printf("no date/file provided. Using Default:\n");
                    printf("%s\n",defaultTime);
                    strcpy(cutoff,defaultTime);
                    printf("beginning recursive walk in %s\n\n",cwd);
                    recursiveWalk(cwd,level,stringToTime(cutoff));
                    return EXIT_SUCCESS;
                }
                
                /*check if argument is a file*/
                printf("argv[2] is not a directory, checking for file:\n");
                char argFile[BUFFER_SIZE];
                snprintf(argFile,sizeof(argFile),"%s/%s", cwd, argv[2]);
                strcpy(argFile, argv[2]);
                int exists = checkFile(argFile);
                if (exists)
                {
                    printf("File provided: %s\n",argFile);
                    char confirmPrint[BUFFER_SIZE];
                    stat(argFile,&argPath);
                    time_t  outT    = argPath.st_mtime;
                    if(S_ISREG(argPath.st_mode))
                    {
                        strftime(confirmPrint, sizeof(confirmPrint), 
                        "%b %d %H:%M", localtime(&argPath.st_mtime));
                    }
                    else
                    {
                        printf("Cannot verify REG file.\n");
                        printf("Returning to terminal\n\n");
                        return EXIT_FAILURE;
                    }
                    printf("EXISTS\n");
                    printf("Using last modified date of: ");
                    printf("%s\n",confirmPrint);
                    printf("beginning recursive walk in %s\n\n",cwd);
                    recursiveWalk(cwd,level,outT);
                    return EXIT_SUCCESS;
                }

                /*check for string-format date time*/
                printf("argv[2] neither directory or file.\n");
                printf("Checking for single-quote delimited string:\n");
                if(stringCheck(argIn))
                    return EXIT_FAILURE;

                printf("\tCorrect digit format detected\n");
                printf("\tDate entered: %s\n",argIn);
                strcpy(cutoff, argIn);
                printf("beginning recursive walk in %s\n\n",cwd);
                recursiveWalk(cwd,level,stringToTime(cutoff));
                return EXIT_SUCCESS;
                
                break;
            
            default:    /* '?' */
                printf("no/unknown flag: reverting to default switch\n");
                printf("Please check formatting of arguments\n");
                printf("'backupfiles -h' displays help\n\n");
                return EXIT_FAILURE;
        }
    
    printf("Incorrect use of arguments.\n\t-h displays help.\nreturning to "
        "terminal\n");
    return EXIT_FAILURE;    
}
