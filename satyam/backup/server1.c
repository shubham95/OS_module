// Server side C/C++ program to demonstrate Socket programming
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <errno.h>

static int PORT = 2325;
static char *dir_name = "connoisseur_dir420";
static char *base_path = "/tmp/dir420";
void *handle_connection(void *param);
void readXBytes(int socket, unsigned int x, void *buffer);
FILE* fopen_mkdir( char *path, char *mode );
struct params
{
    int sockfd;
};

int main(int argc, char const *argv[])
{
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                   &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address,
             sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    else
    {
        printf("Listening on port %d \r\n", PORT);
        while (1)
        {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                                     (socklen_t *)&addrlen)) < 0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            printf("Connection accepted\n");
            pthread_t child;
            struct params *p;
            p = (struct params *)malloc(sizeof(struct params));
            p->sockfd = new_socket;
            pthread_create(&child, NULL, handle_connection, p);
        }
    }
    return 0;
}

void readXBytes(int socket, unsigned int x, void *buffer)
{
    int bytesRead = 0;
    int result;
    while (bytesRead < x)
    {
        result = read(socket, buffer + bytesRead, x - bytesRead);
        if (result < 1)
        {
            // Throw your error.
        }
        bytesRead += result;
    }
}

char* concat(const char *s1, const char *s2)
{
    char *result = (char *) malloc(strlen(s1) + strlen(s2) + 1); // +1 for the null-terminator
    // in real code you would check for errors in malloc here
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

void rek_mkdir(char *path)
{
  char *sep = strrchr(path, '/' );
  if(sep != NULL) {
    *sep = 0;
    rek_mkdir(path);
    *sep = '/';
  }
  printf("Creating folder [%s]",path);
  if(strlen(path)>0 && mkdir(path,0777) && errno != EEXIST )
    printf("error while trying to create '%s'\n%m\n",path ); 
}

FILE* fopen_mkdir( char *path, char *mode )
{
    char *sep = strrchr(path, '/' );
    if(sep ) { 
       char *path0 = strdup(path);
       path0[ sep - path ] = 0;
       rek_mkdir(path0);
       free(path0);
    } 
    return fopen(path,mode);
}

char** split(char *filepath, char* delim)
{
    //Tokenize it into parts
    char *token;
    char *token_list[1024]; //Max elements in file path.
    int path_index_start;
    int total_tokens;
    while ((token = strtok_r(filepath, delim, &filepath)))
    {
        token_list[total_tokens++] = token;
        printf("%s\n", token);
        if (strncmp(token, dir_name, strlen(dir_name)))
            path_index_start = total_tokens;
    }
}

void *handle_connection(void *param)
{
    printf("Inside child\n");
    struct params *p = (struct params *)param;
    int sockfd = p->sockfd;
    char *filepath = (char *)malloc(1024);
    printf("Inside child : fd[%d]\n",sockfd);
    //Read 1024 bytes first
    readXBytes(sockfd, 1024, filepath);
    
    char* sp = strstr(filepath,dir_name);
    sp = sp+strlen(dir_name);
    char* dest_path = concat(base_path,sp);
    printf("[%s]\n",dest_path);
    FILE* f = fopen_mkdir(dest_path,"w");

    int valread = 0;
    char buffer[1024];
    while ((valread = read(sockfd, buffer, 1024)))
    {
        fwrite(buffer,1,valread,f);
        printf("%s\n", buffer);
    }
    
    fclose(f);
}