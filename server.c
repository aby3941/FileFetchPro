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
#include <sys/wait.h>
#include <pwd.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include<time.h>

/* the main sever handles which server will handle load balancing*/

//to keep the count of the clients
int client_connection_count = 0 ;

//command struct to keep the parse details from command.
struct command_node {
    int argc;
    char argv[1000][1000];
	char symbol[9];
};

int empty_file;

//check if the file is empty
bool isemptyfile(char filename[1000]){
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(filename, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    while ((read = getline(&line, &len, fp)) != -1) {
        return false;
    }

    fclose(fp);
    if (line)
        free(line);
    return true;

}

//check if only one file is present in file
bool is_only_one_line(char filename[1000])
{
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(filename, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);
    int count =0;
    while ((read = getline(&line, &len, fp)) != -1) {
        count++;
    }
    if (count == 1)
        return false;

    fclose(fp);
    if (line)
        free(line);
    return true;
}

//read the first line from file
char* readline(char filename[1000])
{
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(filename, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    while ((read = getline(&line, &len, fp)) != -1) {
        return line;
    }

    fclose(fp);
    if (line)
        free(line);
    return "File not found";
}

//read the second line from file
char* read_second_line(char filename[1000])
{
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(filename, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);
    int count =0;
    while ((read = getline(&line, &len, fp)) != -1) {
        if (count == 1)
            return line;
        count++;
    }

    fclose(fp);
    if (line)
        free(line);
    return "File not found";
}

//Removing extra spaces
void trim(char * s) {
	char * p = s;
	int l = strlen(p);

	while(isspace(p[l - 1])) p[--l] = 0;
	while(* p && isspace(* p)) ++p, --l;

	memmove(s, p, l + 1);
}

//parse the command send from client
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

//process client commands
void processclient(int csd1){

    //checking connection based on condition
    if(client_connection_count < 4 || (client_connection_count > 7 && ((client_connection_count % 2) == 0)))
    {
        write(csd1, "ack", 3);    
    } else {
        write(csd1, "nac", 3);
        exit(0);
    }
    int n, pid;
    write(csd1, "Connected to main server", strlen("Connected to main server"));
    printf("connected to client number:%d\n", client_connection_count);
    while(1)
    {            
        char command_string[1000]="", command_copy[1000];
        n=read(csd1, command_string, 1000);
        command_string[n]='\0';
        struct command_node * command = parse_command(command_string);
        char output_filename[1000];
        char output_tar_filename[1000];
        sprintf(output_filename, "server_output_file_%d", client_connection_count);
        sprintf(output_tar_filename, "temp_%d.tar.gz", client_connection_count);
        
        char *output;
        if(strcmp(command->argv[0],"findfile") ==0 )
        {
            //using find command to get the file details
            char exec_command[2000];
            if(!fork())
            {   
                
                sprintf(exec_command, "find ~ -name \"%s\" -exec ls -lh --time-style=\"+%%Y-%%m-%%d\" {} \\; > server_output_file_%d", command->argv[1], client_connection_count);
                printf("%s\n",exec_command);
                execl("/bin/sh", "sh", "-c", exec_command, (char *) NULL);
            }
            wait(NULL);

            if(isemptyfile(output_filename))
            {
                write(csd1, "File not found\n", strlen("File not found\n"));
                continue;
            }
            output = readline(output_filename);
            char formatted_output[1000]="";
            char output_file_name[1000];
            char output_date[1000];
            char output_size[1000];
            char *temp = strtok(output, " ");
            int count = 0;
            strcat(formatted_output, "file size, file created date, file name: ");
            while (temp!=NULL)
            {
                if(count == 4 || count == 5){
                    strcat(formatted_output, temp);
                    strcat(formatted_output, ", ");
                }
                if(count == 6 ){
                    strcat(formatted_output, temp);
                }
                temp = strtok(NULL, " ");
                count++;
            }
            
            write(csd1, formatted_output, strlen(formatted_output));
            unlink(output_filename);
            unlink(output_tar_filename);
            continue;
        } 
        else if (strcmp(command->argv[0],"sgetfiles") ==0 )
        {   
            //use find to copy the files into a temp folder and then create tar using -size
            char exec_command[3000];
            if(!fork())
            {
                int number1, number2;
                number1=atoi(command->argv[1]);
                if(number1 !=0)
                    number1--;
                number2=atoi(command->argv[2])+1;

                char num1[1000], num2[1000];
                sprintf(num1, "%d", number1);
                sprintf(num2, "%d", number2);
                
                sprintf(exec_command, "mkdir -p server_output_folder_%d; find ~ -size +%sc -size -%sc -type f -exec cp -n {} server_output_folder_%d \\;", client_connection_count, num1, num2, client_connection_count);
                printf("%s\n",exec_command);
                execl("/bin/sh", "sh", "-c", exec_command, (char *) NULL);
            }
            wait(NULL);
            char exec_command_1[2000];
            if(!fork())
            {
                sprintf(exec_command_1, "tar -cvzf temp_%d.tar.gz -C server_output_folder_%d/ . > server_output_file_%d ; rm -r -f server_output_folder_%d/", client_connection_count, client_connection_count, client_connection_count, client_connection_count);
                printf("%s\n",exec_command_1);
                execl("/bin/sh", "sh", "-c", exec_command_1, (char *) NULL);
            }
            wait(NULL);
            int fd;
            fd = open(output_tar_filename, O_RDONLY);
            long filesize  = lseek(fd,0,SEEK_END);
            char size[100];
            sprintf(size, "%ld", filesize);
            write(csd1, size, strlen(size));
            char buff[2000];
            lseek(fd,0,SEEK_SET);
            sleep(1);
            for(long i=0; i<filesize; i++){
                read(fd, buff, 1);
                write(csd1, buff, 1);    
            }
            unlink(output_filename);
            unlink(output_tar_filename);
            continue;
        }
        else if (strcmp(command->argv[0],"dgetfiles") ==0 )
        {
            char date2[1000];
            int day1=0, month1=0, year1=0, day2=0, month2=0, year2=0, chars_parsed=0;
            sscanf(command->argv[2], "%4d-%2d-%2d%n", &year1, &month1, &day1, &chars_parsed);
            struct tm dt;

            dt.tm_year = year1 - 1900;
            dt.tm_mon = month1 - 1;
            dt.tm_mday = day1;
            time_t t1 = mktime(&dt);
            t1=t1+(60*60*24);
            struct tm *dt1 = localtime(&t1);
            strftime(date2, 1000, "%y-%m-%d", dt1);

            //use find to copy the files into a temp folder and then create tar using -newerct
            char exec_command[3000];
            if(!fork())
            {
                sprintf(exec_command, "mkdir -p server_output_folder_%d; find ~ -newerct %s ! -newerct %s -type f -exec cp -n {} server_output_folder_%d \\;", client_connection_count, command->argv[1], date2, client_connection_count);
                printf("%s\n",exec_command);
                execl("/bin/sh", "sh", "-c", exec_command, (char *) NULL);
            }
            wait(NULL);
            char exec_command_1[2000];
            if(!fork())
            {
                sprintf(exec_command_1, "tar -cvzf temp_%d.tar.gz -C server_output_folder_%d/ . > server_output_file_%d ; rm -r -f server_output_folder_%d/", client_connection_count, client_connection_count, client_connection_count, client_connection_count);
                printf("%s\n",exec_command_1);
                execl("/bin/sh", "sh", "-c", exec_command_1, (char *) NULL);
            }
            wait(NULL);
            int fd;
            fd = open(output_tar_filename, O_RDONLY);
            long filesize  = lseek(fd,0,SEEK_END);
            char size[100];
            sprintf(size, "%ld", filesize);
            write(csd1, size, strlen(size));
            char buff[2000];
            lseek(fd,0,SEEK_SET);
            sleep(1);
            for(long i=0; i<filesize; i++){
                read(fd, buff, 1);
                write(csd1, buff, 1);    
            }
            unlink(output_filename);
            unlink(output_tar_filename);
            continue;
        }
        else if (strcmp(command->argv[0],"getfiles") ==0 )
        {   
            //use find to copy the files into a temp folder and then create tar using -o and appending filename            
            char arguments[1000]="";
            int argc = command->argc;
            if(strcmp(command->argv[argc], "-u") == 0)
                argc--;

            for(int j=1; j< argc-1; j++)
            {
                strcat(arguments, " -name ");
                strcat(arguments, "\"");
                strcat(arguments, command->argv[j]);
                strcat(arguments, "\"");
                strcat(arguments, " -o ");
            }
            strcat(arguments, " -name ");
            strcat(arguments, "\"");
            strcat(arguments, command->argv[argc-1]);
            strcat(arguments, "\"");
            char exec_command[3000];
            if(!fork())
            {
                sprintf(exec_command, "mkdir -p server_output_folder_%d; find ~ \\( %s \\) -type f -exec cp -n {} server_output_folder_%d \\;", client_connection_count, arguments, client_connection_count);
                printf("%s\n",exec_command);
                execl("/bin/sh", "sh", "-c", exec_command, (char *) NULL);
            }
            wait(NULL);
            char exec_command_1[2000];
            if(!fork())
            {
                sprintf(exec_command_1, "tar -cvzf temp_%d.tar.gz -C server_output_folder_%d/ . > server_output_file_%d ; rm -r -f server_output_folder_%d/", client_connection_count, client_connection_count, client_connection_count, client_connection_count);
                printf("%s\n",exec_command_1);
                execl("/bin/sh", "sh", "-c", exec_command_1, (char *) NULL);
            }
            wait(NULL);
            if(!is_only_one_line(output_filename))
            {   
                write(csd1, "File not found", strlen("File not found"));
                continue;
            } else {
                write(csd1, "File found", strlen("File found"));
            }
            sleep(1);
            int fd;
            fd = open(output_tar_filename, O_RDONLY);
            long filesize  = lseek(fd,0,SEEK_END);
            char size[100];
            sprintf(size, "%ld", filesize);
            write(csd1, size, strlen(size));
            char buff[2000];
            lseek(fd,0,SEEK_SET);
            sleep(1);
            for(long i=0; i<filesize; i++){
                read(fd, buff, 1);
                write(csd1, buff, 1);    
            }
            unlink(output_filename);
            unlink(output_tar_filename);
            continue;
        }
        else if (strcmp(command->argv[0],"gettargz") ==0 )
        {
            //use find to copy the files into a temp folder and then create tar using -o and appending extension            
            char arguments[1000]="";
            int argc = command->argc;
            if(strcmp(command->argv[argc], "-u") == 0)
                argc--;

            for(int j=1; j< argc-1; j++)
            {
                strcat(arguments, " -name ");
                strcat(arguments, "\"");
                strcat(arguments, "*.");
                strcat(arguments, command->argv[j]);
                strcat(arguments, "\"");
                strcat(arguments, " -o ");
            }
            strcat(arguments, " -name ");
            strcat(arguments, "\"");
            strcat(arguments, "*.");
            strcat(arguments, command->argv[argc-1]);
            strcat(arguments, "\"");
            char exec_command[3000];
            if(!fork())
            {
                sprintf(exec_command, "mkdir -p server_output_folder_%d; find ~ \\( %s \\) -type f -exec cp -n {} server_output_folder_%d \\;", client_connection_count, arguments, client_connection_count);
                printf("%s\n",exec_command);
                execl("/bin/sh", "sh", "-c", exec_command, (char *) NULL);
            }
            wait(NULL);
            char exec_command_1[2000];
            if(!fork())
            {
                sprintf(exec_command_1, "tar -cvzf temp_%d.tar.gz -C server_output_folder_%d/ . > server_output_file_%d ; rm -r -f server_output_folder_%d/", client_connection_count, client_connection_count, client_connection_count, client_connection_count);
                printf("%s\n",exec_command_1);
                execl("/bin/sh", "sh", "-c", exec_command_1, (char *) NULL);
            }
            wait(NULL);
            if(!is_only_one_line(output_filename))
            {   
                write(csd1, "File not found", strlen("File not found"));
                continue;
            } else {
                write(csd1, "File found", strlen("File found"));
            }
            sleep(1);
            int fd;
            fd = open(output_tar_filename, O_RDONLY);
            long filesize  = lseek(fd,0,SEEK_END);
            char size[100];
            sprintf(size, "%ld", filesize);
            write(csd1, size, strlen(size));
            char buff[2000];
            lseek(fd,0,SEEK_SET);
            sleep(1);
            for(long i=0; i<filesize; i++){
                read(fd, buff, 1);
                write(csd1, buff, 1);    
            }
            unlink(output_filename);
            unlink(output_tar_filename);
            continue;
        }
        else if (strcmp(command->argv[0],"quit") == 0 )
        {
            exit(0);
        }
    }
}


int main(int argc, char *argv[]){
    int sd, csd, portNumber, status;
    socklen_t len;
    struct sockaddr_in servAdd;
    if(argc != 2){
        printf("Call model: %s <Port #>\n", argv[0]);
        exit(0);
    }
    if((sd = socket(AF_INET, SOCK_STREAM, 0))<0){
    printf("Cannot create socket\n");
    exit(1);
    }
    servAdd.sin_family = AF_INET;
    servAdd.sin_addr.s_addr = htonl(INADDR_ANY);

    sscanf(argv[1], "%d", &portNumber);

    servAdd.sin_port = htons((uint16_t)portNumber); 


    bind(sd,(struct sockaddr*)&servAdd,sizeof(servAdd));
    listen(sd, 5);

    while(1){
        csd = accept(sd,(struct sockaddr *)NULL,NULL);
        if(!fork())
            processclient(csd);
        close(csd);
        client_connection_count++;
        waitpid(0, &status, WNOHANG);
    }
}


