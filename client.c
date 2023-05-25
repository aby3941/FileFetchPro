#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/signal.h>
#include <wait.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>
#include <stdbool.h>
#include <time.h>
#include <sys/wait.h>
#include <pwd.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>

//struc to hold the parsed command details
struct command_node {
    int argc;
    char argv[1000][1000];
	char symbol[9];
};

void usage(char message[1000])
{
	printf("wrong usage: %s\n", message);
}

//Removing extra spaces
void trim(char * s) {
	char * p = s;
	int l = strlen(p);

	while(isspace(p[l - 1])) p[--l] = 0;
	while(* p && isspace(* p)) ++p, --l;

	memmove(s, p, l + 1);
}

// functions to parse the commands into struct
void * parse_command(char command[1000]){
    struct command_node * command_node;
    command_node = (struct command_node*)malloc(sizeof(struct command_node));
    command_node->argc = 0;
    char * temp = strtok(command, " ");
    while (temp != NULL)
    {   
        trim(temp);

        strcpy(command_node->argv[command_node->argc], temp);
        command_node->argc++;
        temp = strtok(NULL, " ");
    }
    return command_node;
}

//check if the string is a number
bool isnumber(char str[1000]){
    for (int i = 0; i < strlen(str); i++)
    {
        if(isdigit(str[i]) == 0)
        return false;
    }
    return true;
}

//check if string is a valid date
bool isdate(char date[1000]){
    struct tm dt;
    int day=0, month=0, year=0, chars_parsed=0;

    if (sscanf(date, "%4d-%2d-%2d%n", &year, &month, &day, &chars_parsed) != 3)
        return false;

    /* If the whole string is parsed, chars_parsed points to the NUL-byte
    after the last character in the string.
    */



    if (date[chars_parsed] != 0)
        return false;
    
    if (month < 1 || month > 12)
        return false;
    
    if (day < 1 || day > 31)
        return false;
    
    return true;
}

//check if date1 >= date2
bool compare_date(char date1[1000], char date2[1000])
{
    struct tm dt;
    char buffer[80];
        int day1=0, month1=0, year1=0, day2=0, month2=0, year2=0, chars_parsed=0;

    if (sscanf(date1, "%4d-%2d-%2d%n", &year1, &month1, &day1, &chars_parsed) != 3)
        return false;

    if (sscanf(date2, "%4d-%2d-%2d%n", &year2, &month2, &day2, &chars_parsed) != 3)
        return false;

    if(year1 > year2)
    {
        return false;
    } else if (year1 < year2)
    {
        return true;
    }
    if(month1 > month2)
    {
        return false;
    } else if (month1 < month2)
    {
        return true;
    }
    if(day1 > day2)
    {
        return false;
    } else if (day1 < day2)
    {
        return true;
    }
    return true;
}

int main(int argc, char *argv[]){
    char message[2000];
    int sid, portNumber, pid, n, s1, s2;
    socklen_t len;
    struct sockaddr_in servAdd;
    // port number 1 should be main server second port number shoudl be mirror server
    if(argc != 4){
        printf("Call model: %s <IP> <Port main server#> <Port mirror server#>\n", argv[0]);
        exit(0);
    }
    //socket for main server
    if((s1=socket(AF_INET, SOCK_STREAM, 0))<0){
        fprintf(stderr, "Cannot create socket\n");
        exit(1);
    }
    //socket for mirror server
    if((s2=socket(AF_INET, SOCK_STREAM, 0))<0){
        fprintf(stderr, "Cannot create socket\n");
        exit(1);
    }
    
    servAdd.sin_family = AF_INET;
    sscanf(argv[2], "%d", &portNumber);
    servAdd.sin_port = htons((uint16_t)portNumber);

    if(inet_pton(AF_INET, argv[1],&servAdd.sin_addr)<0){
        fprintf(stderr, " inet_pton() has failed\n");
        exit(2);
    }
    //connect to main server
    if(connect(s1, (struct sockaddr *) &servAdd, sizeof(servAdd))<0){
        fprintf(stderr, "connect() has failed, exiting\n");
        exit(3);
    }

    sscanf(argv[3], "%d", &portNumber);
    servAdd.sin_port = htons((uint16_t)portNumber);
    
    //connect to mirror server
    if(connect(s2, (struct sockaddr *) &servAdd, sizeof(servAdd))<0){
        fprintf(stderr, "connect() has failed, exiting\n");
        exit(3);
    }
    
    //cehck for ack from main server if ack send request to main else request to mirror
    read(s1, message, 3);
    if(strcmp(message, "ack") == 0)
    {
        sid = s1;
        close(s2);
    } else if(strcmp(message, "nac") == 0)
    {
        sid = s2;
        close(s1);
    }
    read(sid, message, 255);
    fprintf(stderr, "Message Received from Server: %s\n", message);
    while(1)
    {            
        char command_string[1000], command_copy[1000];
        printf("C$ ");
        fflush(stdout);
        fgets(command_string, sizeof(command_string), stdin);
        command_string[strcspn(command_string, "\n")] = 0;
        strcpy(command_copy, command_string);
        struct command_node * command = parse_command(command_string);
        if(command->argc==0)
            continue;
        int flag = 0;

        //check if -u is the last argument
        for(int k=1; k<command->argc-1; k++)
        {
            if(strcmp(command->argv[k],"-u") ==0 )
            {
                printf("-u should be the last argument\n");
                flag =1;
            }
        }
        if(flag == 1)
            continue;

        //input validation
        if(strcmp(command->argv[0],"findfile") ==0 )
        {
            if(command->argc!=2)
            {
                usage("findfile [filename]");
                continue;
            }
        } else if (strcmp(command->argv[0],"sgetfiles") ==0 )
        {   
            if(strcmp(command->argv[command->argc-1],"-u") == 0)
            {
                if(command->argc!=4)
                {
                    usage("sgetfiles [size1] [size2] <-u>");
                    continue;
                }
            }
            else
            {
                if(command->argc!=3)
                {
                    usage("sgetfiles [size1] [size2] <-u>");
                    continue;
                }
            }
            if(!isnumber(command->argv[1]) || !isnumber(command->argv[2]))
            {
                printf("size should be a number\n");
                usage("sgetfiles [size1] [size2] <-u>");
                continue;

            }
            int number1, number2;
            number1=atoi(command->argv[1]);
            number2=atoi(command->argv[2]);
            
            if(number1<0 || number2 <0){
                printf("size should be greater than 0\n");
                usage("sgetfiles [size1] [size2] <-u>");
                continue;
            }
            if(number1>number2){
                printf("size1 should not be greter than size2\n");
                usage("sgetfiles [size1] [size2] <-u>");
                continue;                
            }
        }else if (strcmp(command->argv[0],"dgetfiles") ==0 )
        {
            if(strcmp(command->argv[command->argc-1],"-u") == 0)
            {
                if(command->argc!=4)
                {
                    usage("dgetfiles [date1] [date2] <-u>");
                    continue;
                }
            }
            else
            {
                if(command->argc!=3)
                {
                    usage("dgetfiles [date1] [date2] <-u>");
                    continue;
                }
            }
            if(!isdate(command->argv[1]) || !isdate(command->argv[2]))
            {
                printf("date should be in valid format: yyyy-mm-dd\n");
                usage("dgetfiles [date1] [date2] <-u>");
                continue;

            }
            if(!compare_date(command->argv[1], command->argv[2]))
            {
                printf("date2 should be >= date1\n");
                usage("dgetfiles [date1] [date2] <-u>");
                continue;                
            }
        }else if (strcmp(command->argv[0],"getfiles") ==0 )
        {
            if(strcmp(command->argv[command->argc-1],"-u") == 0)
            {   
                if(command->argc<3 || command->argc>8)
                {
                    usage("getfiles <file list> <-u>");
                    continue;
                }
            }
            else
            {
                if(command->argc<2 || command->argc>7)
                {
                    usage("getfiles <file list> <-u>");
                    continue;
                }
            }
        }else if (strcmp(command->argv[0],"gettargz") ==0 )
        {
            if(strcmp(command->argv[command->argc-1],"-u") == 0)
            {
                if(command->argc<3 && command->argc>8)
                {
                    usage("gettargz <extension list> <-u>");
                    continue;
                }
            }
            else
            {
                if(command->argc<2 && command->argc>7)
                {
                    usage("gettargz <extension list> <-u>");
                    continue;
                }
            }
        }else if (strcmp(command->argv[0],"quit") == 0 )
        {
            if(command->argc!=1)
            {
                usage("quit");
                continue;
            }
            write(sid, command_copy, strlen(command_copy)+1);
            exit(1);
        } else {
            printf("Not a valid command\n");
            continue;
        }

        //send command to server
        write(sid, command_copy, strlen(command_copy)+1);


        if (strcmp(command->argv[0],"findfile") == 0 )
        {
            //read line from server for file details
            if(n=read(sid, message, 255)){
                message[n]='\0';
                printf("%s", message);
                fflush(stdout);
            } 
            continue;
        } else if (strcmp(command->argv[0],"getfiles") == 0 || strcmp(command->argv[0],"gettargz") == 0)
        {
            //read line from server to check if files are found
            if(n=read(sid, message, 255)){
                message[n]='\0';
                if (strcmp(message,"File not found") == 0 ){
                    printf("File not found\n");
                    continue;
                }
            } 
        }
        
        //creating tar.gz and copying content sent from server to client
        n=read(sid, message, 255);
        message[n]='\0';
        long int filesize = atoi(message);
        char buff;
        char content[5]="";
        unlink("temp.tar.gz");
        int fd = open("temp.tar.gz", O_CREAT | O_WRONLY, 0777);
        for(long i=0; i<filesize; i++){
            int rn= read(sid, content, 1);
            write(fd,content,1);  
        }
        close(fd);
        //uncompress the file in client
        if(strcmp(command->argv[command->argc-1], "-u") == 0)
        {
            char exec_command[3000];
            if(!fork())
            {   
                sprintf(exec_command, "tar -xzf temp.tar.gz . > client_dump");
                execl("/bin/sh", "sh", "-c", exec_command, (char *) NULL);
            }
            wait(NULL);
        }
        continue;
    }
}//End main

