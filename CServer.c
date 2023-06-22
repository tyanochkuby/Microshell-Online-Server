#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>


#define max_n 2048
#define RESET "\033[0m"
#define BLUE "\033[36;40m"
#define GREEN "\033[32;40m"
#define DOLLAR  " $ "

char workdir[max_n];
char *words[] = {"cd", "ls", "help", "exit", "rm", "download"};



int spaces(char *str)
{
    int count = 0, i;
    for(i = 0; i < strlen(str); i++)
        if(str[i] == ' ')
            count++;
    return count;
}

void splitstring (char* args[], char* str, char splitter)
{
    char *token;
    args[spaces(str)+1] = NULL;
    int i = 1;
    token = strtok(str, " ");
    args[1] = token;
    while ((token = strtok(NULL, " ")) != NULL)
        args[++i] = token;
}

void help(int connfd)
{
    char help[] = "Microshell online by Nastassia Zhuravel and Roman Judeszko \nOur commands: \nls \nhelp \ncd \nrm \nexit \ndownload\n";
    if(send(connfd, help, sizeof(help), 0) != sizeof(help))
        fprintf(stderr, "send: %s (%d)\n", strerror(errno), errno);
}

void strcop(char* str1, char* str2)
{
    int i, size1 = strlen(str1), size2 = strlen(str2);
    for(i = size1; i < size1 + size2; i++)
        str1[i] = str2[i - size1];
}



void cd_functilda(char *str)
{
    if(str[0] == '~')
    {
        int i;
        char a[max_n];
        for(i = 0; i < max_n; i++)
            a[i] = '\0';
        strcop(a, "/home/students");
        strcop(a, getlogin());
        int outsize = strlen(a), fromsize = strlen(str);
        for(i = outsize; i < fromsize + outsize - 1; i++)
        {
            a[i] = str[i - outsize + 1];
        }
        strcpy(str, a);
        str[strlen(str)] = '\0';
    }
}

void rem(char* file, int connfd)
{
    if (-1 == remove (file))
        send(connfd, "Error", 5, 0);
    else
        send(connfd, "Success", 7, 0);
    return;
}

void ls(char *args, int connfd) {
    DIR* dir;
    char output[max_n];
        memset(output, 0, max_n);
    if(args != NULL)
    {
        
        cd_functilda(args);
        dir = opendir(args);
        if(dir == 0)
        {
            strcop(output, args);
            strcop(output, ": there is no such directory");
            send(connfd, output, strlen(output), 0);
            return ;
        }
    }
    else
        dir = opendir(".");
    struct dirent* entity = readdir(dir);
    char buf[max_n][max_n];
    int i = 0;
    while (entity != NULL)
    {
        if(entity->d_type == 4)
        {
            if(strcmp(entity->d_name, ".") && strcmp(entity->d_name, ".."))
            {
                strcop(output, BLUE);
                strcop(output, entity->d_name);
                strcop(output, RESET);
                strcop(output, "\n");
            }
        }

        else
        {
            strcpy(buf[i], entity->d_name);
            i++;
        }

        entity = readdir(dir);
    }

    int k;
    for(k = 0; k < i; k++)
    {
        strcop(output, buf[k]);
        strcop(output, "\n");
    }
    closedir(dir);
    send(connfd, output, strlen(output), 0);
}

void cd_func(char* args, int connfd)
{
    char err[] = "cd: There is no such directory\n";
    if(args == NULL)
    {
        memset(args, 0, max_n);
        args[0] = '~';
    }
    cd_functilda(args);
    if(chdir(args) == -1)
        send(connfd, err, strlen(err), 0);
    else
        send(connfd, "void", 4, 0);
}

void download(char* path, int gk)
{
    long filelen, sent = 0, senttotal = 0, read;
    int slash_pos;
    struct stat fileinfo;
    FILE* file;
    unsigned char buf[max_n];

    if(path != NULL)
    {
        if (stat(path, &fileinfo) < 0)
        {
            printf("Child: file info getting error\n");
            return;
        }
        if (fileinfo.st_size == 0)
        {
            printf("Child: file size: 0\n");
            return;
        }
        printf("Chlid: file size: %d\n", fileinfo.st_size);

        filelen = htonl((long) fileinfo.st_size);

        if (send(gk, &filelen, sizeof(long), 0) != sizeof(long))
        {
            printf("Child: file size sending error\n");
            return;
        }
        if(recv(gk, buf, max_n, 0) <= 0)
        {
            printf("Child: answer receiving error\n");
            return;
        }
        buf[strlen(buf)] = 0;
        if(!strcmp("y", buf) || !strcmp("Y", buf))
        {
            memset(buf, 0, max_n);

            filelen = fileinfo.st_size;
            file = fopen(path, "rb");
            if(file == 0)
            {
                printf("Child: file reading error\n");
                return;
            }
            while (senttotal < filelen)
            {
                read = fread(buf, 1, 1024, file);
                sent = send(gk, buf, read, 0);
                if (read != sent)
                    break;
                senttotal += sent;
                //printf("Child: %d bytes sent\n", senttotal);
            }

            if (senttotal == filelen)
                printf("Child: file sent succesfully\n");
            else
                printf("Child: file sending error");
            fclose(file);
        }
        return;
    }
    else
    {
        printf("Please enter file name you want to download:\n");
        
    }

}

void sw(char *program, char *args, int connfd) {
    int i;
    for (i = 0; i < sizeof words/sizeof words[0]; i++)
    {
        if (!strcmp(program, words[i]))
        {
            switch(i)
            {
                case 0:
                    if(args == NULL)
                    {
                        cd_func(NULL, connfd);
                        break;
                    }
                    cd_func(args, connfd);
                    break;
                case 1:
                    ls(args, connfd);
                    break;
                case 2:
                    help(connfd);
                    break;
                case 3:
                    send(connfd, "bye", 3, 0);
                    exit(0);
                    break;
                case 4:
                    rem(args, connfd);
                    break;
                case 5:
                    download(args, connfd);
                    break;

            }
            return ;
        }
    }

    int link[2];
    char foo[4096];
    pipe(link);
    pid_t id = fork();
    if (id == 0)
    {
        dup2 (link[1], STDOUT_FILENO);
        close(link[0]);
        close(link[1]);
        if(args == NULL){
            char* forexec[2] = {program, NULL};
            if(execvp(program, forexec) == -1)
                printf("There is no such program sorry :/ \n");
        }
        else
        {
            char* forexec[spaces(args) + 3];
            splitstring(forexec, args, ' ');
            forexec[spaces(args) + 2] = NULL;
            forexec[0] = program;
            if(execvp(program, forexec) == -1)
                printf("There is no such program sorry :/ \n");
        }

    }
    else
        close(link[1]);
        int nbytes = read(link[0], foo, sizeof(foo));
        if(!foo[0]=='\0')
            send(connfd, foo, nbytes, 0);
        else
            send(connfd, "void", 4, 0);
        wait(NULL);
    return;
}




void Serve(int connfd)
{
    char command[max_n], out[max_n], workdir[max_n], program[max_n], arguments[max_n];
    int space_pos, size, i;



    chdir(getenv("HOME"));
    getcwd(workdir, max_n);
    printf("%s\n",workdir);

    help(connfd);

    while(1)
    {
        strcop(out, GREEN);
        getcwd(workdir, max_n);
        strcop(out, workdir);
        strcop(out, RESET);
        strcop(out, DOLLAR);
        memset(command, 0, max_n);
        if (send(connfd, out, strlen(out), 0) != strlen(out))
        {
            fprintf(stderr, "send: %s (%d)\n", strerror(errno), errno);
            //printf("Path send error\n");
            return ;
        }

        if(recv(connfd, command, max_n, 0) <= 0)
        {
            printf("Command receive error\n");
            continue;
        }

        command[strcspn(command, "/n")] = 0;
        size = strlen(command);

        if(strchr(command, ' ') != 0)
        {
            char *prc = strchr(command, ' ');
            space_pos = prc - command;


            for(i = 0; i < space_pos; i++)
            {
                program[i] = command[i];
            }
            for(i = space_pos+1; i < size; i++)
            {
                arguments[i-space_pos-1] = command[i];
            }
            sw(program, arguments, connfd);
        }
        else if (strlen(command))
            sw(command, NULL, connfd);
        memset(out, 0, max_n);
        memset(workdir, 0, max_n);
    }
}

int main(void)
{
    struct sockaddr_in myadr;
    int port, i, listenfd = 0, connfd = 0, bytesRead = 0;
    char buf[max_n];
    socklen_t dladr = sizeof(struct sockaddr_in);

    printf("Numer portu: ");
    scanf("%d", &port);
    if((listenfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
        printf("Socket creation failed\n");


    myadr.sin_family = AF_INET;
    myadr.sin_port = htons(port);
    myadr.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(myadr.sin_zero, 0, sizeof(myadr.sin_zero));

    if(bind(listenfd, (struct sockaddr*) &myadr, sizeof(struct sockaddr_in)) < 0)
    {
        printf("bind error\n");
        return 1;
    }

    listen(listenfd, 5);

    while(1)
    {
        if(connfd = accept(listenfd, (struct sockaddr*)NULL, NULL) < 0)
        {
            printf("Main: accept error\n");
            continue;
        }
        else
            printf("Main: accept done\n");
        //printf("Connection from %s:%u\n", inet_ntoa(myadr.sin_addr), ntohs(myadr.sin_port));
        if(fork()==0)
        {
            if(bytesRead = recv(connfd, buf, sizeof(buf), 0) == -1)
                fprintf(stderr, "recv: %s (%d)\n", strerror(errno), errno);
            buf[bytesRead] = '\0';
            printf("%s\n", buf);

            printf("Child: starting serve function\n");
            Serve(connfd);
        }
        else
        {
            printf("Main: back to listening\n");
            continue;
        }
    }

    return 0;
}


