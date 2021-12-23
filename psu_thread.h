#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ucontext.h>
#include <pthread.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

void dummy(void *user_args);

//Socket Fields
int cli_sockfd, srv_sockfd, tmp_sockfd;
int portno;
struct sockaddr_in serv_addr;
struct hostent *server;

//Thread Fields
typedef struct psu_thread_info {
	ucontext_t ucontext;
	ucontext_t uctx_main;
	char stack[0x6000];	
} psu_thread_info_t;

pthread_t thr;
psu_thread_info_t thread_info;
ucontext_t uctx_foo;
int xcode;
pthread_attr_t tattr;

void psu_thread_setup_init(int mode)
{
	portno = 8087;
	
	//Server-side socket setup
	if(mode) {
		xcode = 1;
		socklen_t clilen;
		char hostbuf[32];
		struct sockaddr_in serv_addr, cli_addr;
		
		srv_sockfd = socket(AF_INET, SOCK_STREAM, 0);
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		serv_addr.sin_port = htons(portno);
		
		bind(srv_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	
		gethostname(hostbuf, sizeof(hostbuf));	

		listen(srv_sockfd, 3);
		clilen = sizeof(cli_addr);
		tmp_sockfd = accept(srv_sockfd, (struct sockaddr *) &cli_addr, &clilen);
		
		int n = 1;
		int b = 0;
		while(n > 0) {
			n = read(tmp_sockfd, ((char*)&thread_info)+b, sizeof(thread_info));
			b += n;
		}
		
		close(tmp_sockfd);
		close(srv_sockfd);
	}
	//Client-side socket setup	
	else {	
		cli_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	}

	return;
}

void * (*func)(void*);

int psu_thread_create(void * (*user_func)(void*), void *user_args)
{
		
	func = user_func;	
	pthread_create(&thr, &tattr, dummy, user_args);
        pthread_join( thr, NULL);
		
	return 0; 
}

void dummy(void *user_args) {
	
	//Resume context of the previous machine 	
	if(xcode) {
		swapcontext(&thread_info.uctx_main, &thread_info.ucontext);
	}
	//Create new context
	else {
		getcontext(&uctx_foo);
		uctx_foo.uc_stack.ss_sp = thread_info.stack;	
		uctx_foo.uc_stack.ss_size = sizeof(thread_info.stack);
		thread_info.ucontext.uc_link = &thread_info.uctx_main;
		makecontext(&uctx_foo, func, 1, user_args);	
		swapcontext(&thread_info.uctx_main, &uctx_foo);
	}
}

void psu_thread_migrate(const char *hostname)
{

	getcontext(&thread_info.ucontext);
	
	//Return once the execute resumes in the server side
	if(xcode) {
                return;
        }
	 
	//Client side message passing
	server = gethostbyname(hostname);
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,server->h_length);
	serv_addr.sin_port = htons(portno);	
	connect(cli_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));

	write(cli_sockfd, &thread_info, sizeof(thread_info));
	
	close(cli_sockfd);
	swapcontext(NULL, &thread_info.uctx_main);
	return;
}
