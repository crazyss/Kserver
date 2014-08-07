#include "stdio.h"
#include "ksocket.h"
#include "csapp.h"

#include "libvirt.h"
#include <libvirt/libvirt.h>




/*Hypervisor Ptr*/
virConnectPtr conn;

/*
 *
 * get_filetype - derive file type from file name
 */

void get_filetype(char *filename, char *filetype)
{
    if (strstr(filename, ".html"))
        strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
        strcpy(filetype, "image/git");
    else if (strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpeg");
    else
        strcpy(filetype, "text/plain");
}




void serve_dynamic(int fd, char *filename, char *cgiargs)
{
    char buf[MAXLINE], *emptylist[] = {NULL};

    /* Return first part of HTTP response */
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: King Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));
    if (Fork() == 0) { /*child*/
        setenv("QUERY_STRING", cgiargs, 1);
        Dup2(fd, STDOUT_FILENO);
        Execve(filename, emptylist, environ);
    }
    Wait(NULL);
}
void serve_static (int fd, char *filename, int filesize)
{
    int srcfd;
    char *srcp,filetype[MAXLINE], buf[MAXBUF];

    /*Send response headers to client*/
    get_filetype(filename, filetype);
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf, "%sServer: King Web Server\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    Rio_writen(fd, buf, strlen(buf));

    

    srcfd=Open(filename, O_RDONLY, 0);
    srcp=Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    Close(srcfd);
    Rio_writen(fd, srcp, filesize);
    Munmap(srcp, filesize);

}
void hypervisor_display(int fd, char *output, int length)
{
    char buf[MAXBUF];

        sprintf(buf, "HTTP/1.0 200 OK\r\n");
            sprintf(buf, "%sServer: King Web Server\r\n", buf);
            sprintf(buf, "%sContent-length: %d\r\n", buf, length);
             sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, "text/plain");
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, output, length);

    
}


int parse_uri(char *uri, char *filename, char *cgiargs)
{
    char *ptr;
    if (!strstr(uri, "cgi-bin")) {
        strcpy(cgiargs, "");
        strcpy(filename, ".");
        strcat(filename, uri);
        if (uri[strlen(uri)-1] == '/')
            strcat(filename, "home.html");
        return 1;
    }
    else {
        ptr = index(uri, '?');
        if (ptr) {
            strcpy (cgiargs, ptr+1);
            *ptr='\0';
        }
        else {
            strcpy(cgiargs, "");
        }
        strcpy(filename, ".");
        strcat(filename, uri);
        return 0;
    }
}

void read_requesthdrs(rio_t *rp)
{
    char buf[MAXLINE];
    Rio_readlineb(rp, buf, MAXLINE);
    while(strcmp(buf, "\r\n")) {
        Rio_readlineb(rp, buf, MAXLINE);
        /*print the header into the console*/
        fprintf(stdout, "%s", buf);
    }
    return ;
}
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
    char buf[MAXLINE],body[MAXBUF];
    sprintf(body, "<html><title>KServer Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n",body ,errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body ,longmsg, cause);
    sprintf(body, "%s<hr><em>The King Web server</em>\r\n", body);
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);

    /*HTTP response*/
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

void doit(int fd)
{
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE],method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    rio_t rio;

    Rio_readinitb(&rio, fd);
    Rio_readlineb(&rio, buf, MAXLINE);
    sscanf(buf,"%s%s%s", method, uri, version);
    if (strcasecmp(method, "GET")) {
        clienterror(fd, method, "501", "Not Implemented", "KServer does not implement this method");
        return;
    }
    read_requesthdrs(&rio);

    /*Parse URI from GET request*/
    is_static = parse_uri(uri, filename, cgiargs);
    if (strcmp(filename,"hypervisor")) {
        /*Open virt connection*/
        conn = Openvirt_connect();

    /*different data have different ways to display*/
#if 0
        /*If request Hypervisor Information*/
        char *buf = virConnectGetCapabilities(conn);
        hypervisor_display(fd, buf, strlen(buf));
        free(buf);
#endif


        virConnectClose(conn);
    }
    if (stat(filename,&sbuf) < 0) {
        clienterror(fd, filename, "404", "Not found", "KServer couldn't find this file");
        return;
    }
    if (is_static) {
        if (!(S_ISREG(sbuf.st_mode)) || ! (S_IRUSR & sbuf.st_mode)) {
            clienterror(fd, filename, "403", "Forbidden", "KServer couldn't read the file");
            return;
        }
        serve_static(fd,filename,sbuf.st_size);
    }
    else {
        if (!(S_ISREG(sbuf.st_mode)) || ! (S_IRUSR & sbuf.st_mode)) {
            clienterror(fd, filename, "403", "Forbidden", "KServer couldn't run the CGI program");
            return;
        }
        serve_dynamic(fd,filename,cgiargs);
    }
}

int main(int argc,char **argv)
{
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;

    if (argc != 2) {
        fprintf(stderr, "usage:%s <port> \n", argv[0]);
        exit(1);
    }


    port = atoi(argv[1]);
    listenfd = Open_listenfd(port);
    while(1) {
        clientlen = sizeof (clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        doit(connfd);
        Close(connfd);
    }

    return 0;
}
