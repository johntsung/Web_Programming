#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<netinet/in.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<signal.h>
#include<netdb.h>
#include<fcntl.h>

void put_to_html(int fd){
	int i,j,file_fd,input_size;
	char buffer[BUFSIZ + 10];

	input_size=read(fd,buffer,BUFSIZ);
	buffer[input_size]=0;

	char req[20];
	for(i=0;i<20;i++){
		req[i]=buffer[i];
		if(buffer[i]=='\r' || buffer[i]=='\n')
			break;
	}
	req[i]=0;
	if(strncmp(req,"GET",3)==0){
		if(strstr(req,".jpg")){
			sprintf(buffer,"HTTP/1.0 200 OK\r\nContent-Type: image/jpg\r\n\r\n");
			write(fd,buffer,strlen(buffer));
			file_fd=open("image/my_head.jpg",O_RDONLY);
		}
		else{
			strcpy(buffer,"GET /index.html\0");
			file_fd=open("index.html",O_RDONLY);
		}
		while((input_size=read(file_fd,buffer,BUFSIZ))>0){
			write(fd,buffer,input_size);
		}
		close(file_fd);
	}
	if(strncmp(req,"POST",4)==0){  //due the upload case
		int op_fd;
		char *pos=strstr(buffer,"filename=\""),file[512]="upload/";
		pos+=10;i=7;
		char *end=strstr(pos,"\""),*p;
		for(p=pos;p!=end;p++,i++){
			file[i]=*p;
		}
		file[i]=0;
		pos=strstr(pos,"\n");
		pos=strstr(pos+1,"\n");
		pos=strstr(pos+1,"\n");
		pos++;
		op_fd=open(file, O_CREAT | O_WRONLY | O_SYNC, S_IRWXU | S_IRWXG | S_IRWXO);
		write(op_fd,pos,BUFSIZ-(pos-buffer));
		while(input_size==BUFSIZ){
			input_size=read(fd,buffer,BUFSIZ);
			write(op_fd,buffer,input_size);
		} 
		close(op_fd);
		
		strcpy(buffer,"GET /index.html\0");
		file_fd=open("index.html",O_RDONLY);
		while((input_size=read(file_fd,buffer,BUFSIZ))>0){
			write(fd,buffer,input_size);
		}
		close(file_fd);
		write(fd,"The File is upload",18);
	}
	exit(1);
}

void sigchld_handler(int s){
	while(waitpid(-1,NULL,WNOHANG)>0);  //avoid the zombie process
}

int main(void){
	int fir_fd, sec_fd;
	struct addrinfo info, *servinfo, *p;
	struct sockaddr_storage cli_addr;
	socklen_t size;
	struct sigaction sa;
	int yes=-1;
	char s[INET6_ADDRSTRLEN];
	int rv;

	memset(&info, 0, sizeof info);
	info.ai_family = AF_UNSPEC;  //cannot accept IPv4 and IPv6
	info.ai_socktype = SOCK_STREAM;
	info.ai_flags = AI_PASSIVE;  //assign the address local host to socket structure
	info.ai_protocol = 0;

	getaddrinfo(NULL,"80",&info,&servinfo); //get the server infomation
	
	fir_fd=socket(servinfo->ai_family,servinfo->ai_socktype,servinfo->ai_protocol);
	
	setsockopt(fir_fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int));
	
	if(bind(fir_fd,servinfo->ai_addr,servinfo->ai_addrlen)<0){ // 
		perror("bind");
		exit(1);
	}
	
	freeaddrinfo(servinfo);
	
	listen(fir_fd,10);

	sa.sa_handler=sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags=SA_RESTART;

	sigaction(SIGCHLD,&sa,NULL);
	
	while(1){ //Do the concurrent server
		int pid;
		size=sizeof(cli_addr);
		sec_fd=accept(fir_fd, (struct sockaddr*)&cli_addr, &size); //get the client fd
		if(sec_fd<0){
			perror("accept");
			continue;
		}
		if((pid=fork())<0){ 
			perror("fork error");
			exit(0);
		}
		else if(pid==0){
			close(fir_fd);
			put_to_html(sec_fd);
			close(sec_fd);
			exit(0);
		}
		close(sec_fd);
	}
}
