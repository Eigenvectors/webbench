/*
 * Simple forking WWW Server benchmark:
 *
 * Usage:
 *   webbench --help
 *
 * Return codes:
 *    0 - success
 *    1 - benchmark failed (server is not on-line)
 *    2 - bad parameters
 *    3 - internal error, fork failed and so on
 * 
 */ 

#include "webbench.h"

/* values */
volatile int timerexpired = 0;
int speed = 0;
int failed = 0;
int bytes = 0;
/* globals */


int http10 = 1; /* 0 - http/0.9, 1 - http/1.0, 2 - http/1.1 */


/* Allow: GET, HEAD, OPTIONS, TRACE */
#define METHOD_GET 0
#define METHOD_HEAD 1
#define METHOD_OPTIONS 2
#define METHOD_TRACE 3
#define PROGRAM_VERSION "1.5"


int method = METHOD_GET;		//default
int clients = 1;				//simulate only one http client request by default
int force = 0;					//defualt 
int force_reload = 0;			//defualt
int proxyport = 80;
char *proxyhost = NULL;
int benchtime = 30;		//benchmark 30 seconds by default
/* internal */
int mypipe[2];
#define MAXHOSTNAMELEN 2048
char host[MAXHOSTNAMELEN];		//I don't define this macro????????????
#define REQUEST_SIZE 2048
char request[REQUEST_SIZE];		//why request store too much information



//just a static const struct array
static const struct option long_options[] =	
{
	{"force", no_argument, &force, 1},
 	{"reload", no_argument, &force_reload, 1},
 	{"time", required_argument, NULL, 't'},
 	{"help", no_argument, NULL, '?'},
 	{"http09", no_argument, NULL, '0'},			//??????????????????????
 	{"http10", no_argument, NULL, '1'},
 	{"http11", no_argument, NULL, '2'},
 	{"get", no_argument, &method, METHOD_GET},			//care about the four options
 	{"head", no_argument, &method, METHOD_HEAD},
 	{"options", no_argument, &method, METHOD_OPTIONS},
 	{"trace", no_argument, &method, METHOD_TRACE},
 	{"version", no_argument, NULL, 'V'},
 	{"proxy", required_argument, NULL, 'p'},
 	{"clients", required_argument, NULL, 'c'},
 	{NULL, 0, NULL, 0}			//zero case
};



/* functions prototypes declarations */
static void benchcore(const char* host, const int port, const char *request);
static int bench(void);
static void build_request(const char *url);

static void alarm_handler(int signal)
{
   timerexpired = 1;
}	



static void usage(void)
{
   fprintf(stderr,
	"webbench [option]... URL\n"
	"  -f|--force               Don't wait for reply from server.\n"
	"  -r|--reload              Send reload request - Pragma: no-cache.\n"
	"  -t|--time <sec>          Run benchmark for <sec> seconds. Default 30.\n"
	"  -p|--proxy <server:port> Use proxy server for request.\n"
	"  -c|--clients <n>         Run <n> HTTP clients at once. Default one.\n"
	"  -0|--http09              Use HTTP/0.9 style requests.\n"
	"  -1|--http10              Use HTTP/1.0 protocol.\n"
	"  -2|--http11              Use HTTP/1.1 protocol.\n"
	"  --get                    Use GET request method.\n"
	"  --head                   Use HEAD request method.\n"
	"  --options                Use OPTIONS request method.\n"
	"  --trace                  Use TRACE request method.\n"
	"  -?|-h|--help             Get usage information.\n"
	"  -V|--version             Display program version.\n"
	);
}

//pay more attentions to --get --head --options --trace

int main(int argc, char *argv[])
{
	int opt = 0;
 	int options_index = 0;
    char *tmp = NULL;

	if(argc == 1)
 	{
		usage();
        return 2;
 	} 

 	while((opt = getopt_long(argc, argv, "912Vfrt:p:c:?h", long_options, &options_index)) != -1 )		//t p c need extra parameters
 	{
  		switch(opt)
  		{
   			case  0: break;				//force, reload, get, head, options, trace will return 0
   			case 'f': force = 1; break;
   			case 'r': force_reload = 1; break; 
   			case '9': http10 = 0; break;
   			case '1': http10 = 1; break;
   			case '2': http10 = 2; break;
   			case 'V': printf(PROGRAM_VERSION "\n"); exit(0);
   			case 't': benchtime = atoi(optarg); break;	     
   			case 'p': 
	     		/* proxy server parsing server:port */
	     		tmp = strrchr(optarg, ':');
	     		proxyhost = optarg;		//include ip/hostname and port number        ??????????????????
	     		if(tmp == NULL)
	     		{
		     		break;
	     		}
	     		else if(tmp == optarg)
	     		{
		     		fprintf(stderr,"Error in option --proxy %s: Hostname is missing.\n", optarg);
		     		return 2;
	     		}
	     		else(tmp == optarg + strlen(optarg) - 1)
	     		{
		     		fprintf(stderr,"Error in option --proxy %s Port number is missing.\n",optarg);
		     		return 2;
	     		}

	     		*tmp = '\0';
	     		proxyport = atoi(tmp + 1);	//convert C type string into integer
				break;
   			case ':':
   			case 'h':
   			case '?': usage(); return 2; break;			//this break is not necessary
   			case 'c': clients = atoi(optarg); break;
  		}
	 }
 
 	if(optind == argc)		//this code is very clever 
	{
    	fprintf(stderr,"webbench: Missing URL!\n");
		usage();
		return 2;
    }

 	if(clients == 0) clients = 1;     		//the amount of clients is 1 at least
 	if(benchtime == 0) benchtime = 30;		//default 30 front

 	/* Copyright */		//the prompt message when the program is run
 	fprintf(stderr,"Webbench - Simple Website Benchmark tool "PROGRAM_VERSION"\n"
	 "Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.\n" 
	 "I just read the source code and do some little change. That's all.\n");


 	build_request(argv[optind]);	//just initial a request and store relatived information into request array

 	/* print bench info */
 	printf("\nBenchmarking: ");
 	switch(method)
	 {
		 case METHOD_GET:
		 default:
			 printf("GET"); break;
		 case METHOD_OPTIONS:
			 printf("OPTIONS"); break;
		 case METHOD_HEAD:
			 printf("HEAD"); break;
		 case METHOD_TRACE:
			 printf("TRACE"); break;
 	}
 	printf(" %s", argv[optind]);	//if user don't input url, the sentence will output what

 	switch(http10)
	{
		case 0: printf(" (using HTTP/0.9)"); break;
	 	case 2: printf(" (using HTTP/1.1)"); break;
	}
	printf("\n");

	if(clients == 1) printf("1 client");
	else
   		printf("%d clients", clients);
 	printf(", running %d sec", benchtime);
	if(force) printf(", early socket close");
	if(proxyhost != NULL) printf(", via proxy server %s:%d", proxyhost, proxyport);
 	if(force_reload) printf(", forcing reload");
 	printf(".\n");
 	return bench();
}


static void build_request(const char *url)
{
  char tmp[10];
  int i;

  bzero(host, MAXHOSTNAMELEN);
  bzero(request, REQUEST_SIZE);

  if(force_reload && proxyhost != NULL && http10 < 1) http10 = 1;		//I don't understand the reason
  if(method == METHOD_HEAD && http10 < 1) http10 = 1;		//maybe force_reload and head method need http10 at least
  if(method == METHOD_OPTIONS && http10 < 2) http10 = 2;	//maybe options and trace method need http11 at least
  if(method == METHOD_TRACE && http10 < 2) http10 = 2;

  switch(method)
  {
	  default:
	  case METHOD_GET: strcpy(request, "GET"); break;
	  case METHOD_HEAD: strcpy(request, "HEAD"); break;
	  case METHOD_OPTIONS: strcpy(request, "OPTIONS"); break;
	  case METHOD_TRACE: strcpy(request, "TRACE"); break;
  }
		  
  strcat(request, " ");			//use space as delimiter

  if(NULL == strstr(url, "://"))	//url don't include "://"
  {
	  fprintf(stderr, "\n%s: is not a valid URL.\n", url);
	  exit(2);
  }
  if(strlen(url) > 1500)
  {
  	  fprintf(stderr, "URL is too long.\n");
	  exit(2);
  }
  if(proxyhost == NULL)
  {
	   if(0 != strncasecmp("http://", url, 7)) 
	   { 	fprintf(stderr, "\nOnly HTTP protocol is directly supported, set --proxy for others.\n");
            exit(2);
       }
  }
  /* protocol/host delimiter */
  i = strstr(url, "://") - url + 3;		//I think i is 7
  /* printf("%d\n",i); */

	//can don't have port number, can't don't have hostname
  if(strchr(url + i, '/') == NULL)	//ensure end character '/' exist
  {
  	   fprintf(stderr, "\nInvalid URL syntax - hostname don't ends with '/'.\n");	//the host name should ends with '/' character
       exit(2);
  }


  if(proxyhost == NULL)
  {
  	 	/* get port from hostname */		//I need to know the formation of url
       if(index(url + i, ':') != NULL && index(url + i, ':') < index(url + i, '/'))		//':' should before '/'
   	   {
	   		strncpy(host, url + i, strchr(url + i, ':') - url - i);	//get host name from url		this place have some problems????????
	   		bzero(tmp, 10);
	   		strncpy(tmp, index(url + i, ':') + 1, strchr(url + i, '/') - index(url + i, ':') - 1);	//get port number from url
	   		/* printf("tmp=%s\n",tmp); */
	   		proxyport = atoi(tmp);
	   		if(proxyport == 0) proxyport = 80;	//if input ilegal, back to default value
   		} 
   		else
   		{
     		strncpy(host, url + i, strcspn(url + i, "/"));	//user don't supply port number, use defualt port number
   		}
   		// printf("Host=%s\n",host);
   		strcat(request + strlen(request), url + i + strcspn(url + i, "/"));		//store hostname and port number into request array
  } 
  else
  {
   		// printf("ProxyHost=%s\nProxyPort=%d\n",proxyhost,proxyport);
   		strcat(request, url);	//just add '\0'   I think this sentence is not necessary
  }


  if(http10 == 1)
	  strcat(request, " HTTP/1.0");
  else if(http10 == 2)
	  strcat(request, " HTTP/1.1");
  strcat(request, "\r\n");


  if(http10 > 0)
	  strcat(request, "User-Agent: WebBench " PROGRAM_VERSION "\r\n");
  if(proxyhost == NULL && http10 > 0)    //when user don't use proxy server, host only include hostname information
  {
	  strcat(request, "Host: ");
	  strcat(request, host);
	  strcat(request, "\r\n");
  }


  if(force_reload && proxyhost != NULL)   //use proxy server and force_reload work method
  {
	  strcat(request, "Pragma: no-cache\r\n");
  }
  if(http10 > 1)
	  strcat(request, "Connection: close\r\n");
  /* add empty line at end */
  if(http10 > 0) strcat(request, "\r\n"); 
  // printf("Req = %s\n", request);
}


/* vraci system rc error kod */
static int bench(void)
{
  	int i, j, k;	
  	pid_t pid = 0;
  	FILE *f;

  	/* check avaibility of target server */
  	i = Socket(proxyhost == NULL ? host : proxyhost, proxyport);
  	if(i < 0) 
  	{	 
	  	 fprintf(stderr, "\nConnect to server failed. Aborting benchmark.\n");
       	 return 1;
  	}
  	close(i);
  	/* create pipe */
  	if(pipe(mypipe))
  	{
	  	perror("pipe failed.");
	  	return 3;
  	}

  	/* not needed, since we have alarm() in childrens */
  	/* wait 4 next system clock tick */
  	/*
  		cas=time(NULL);
  		while(time(NULL)==cas)
        sched_yield();
  	*/

  	/* fork childs */
  	for(i = 0; i < clients; i++)
  	{
	   	pid = fork();
	   	if(pid <= (pid_t)0)
	   	{
		  	 /* child process or error*/
	       	 sleep(1); /* make childs faster */
		     break;
	   	}
  	}

  	if(pid < (pid_t)0)
  	{
       	 fprintf(stderr, "problems forking worker no. %d\n", i);
	   	 perror("fork failed.");
	     return 3;
  	}

  	if(pid == (pid_t)0)
  	{
  	  	 /* I am a child */
       	if(proxyhost == NULL)
      	   	benchcore(host, proxyport, request);
       	else
      	   	benchcore(proxyhost, proxyport, request);

        /* write results to pipe */
	    f = fdopen(mypipe[1], "w");		//mypipe[0] be used to store read file descriptor, mypipe[1] be used to store write file descriptor
	   	if(f == NULL)
	   	{
		   	perror("open pipe for writing failed.");
		   	return 3;
	   	} 
	  	 /* fprintf(stderr, "Child - %d %d\n", speed, failed); */
	   	fprintf(f, "%d %d %d\n", speed, failed, bytes);	//the child process write the data to a file pointer by f
	   	fclose(f);
	   	return 0;
  	}	  
  	else
  	{	   	//parent process
	   		f = fdopen(mypipe[0], "r");
	   		if(f == NULL) 
	   		{
	 	   		perror("open pipe for reading failed.");
		   		return 3;
	   		}
	   		setvbuf(f, NULL, _IONBF, 0);		//don't use buffer
	   		speed = 0;
       		failed = 0;
       		bytes = 0;
	   		i = 0;
	   		j = 0;
	  		k = 0;

	   		while(1)
	   		{
		   		pid = fscanf(f, "%d %d %d", &i, &j, &k);
		   		if(pid < 2)
           		{
          	    	fprintf(stderr, "Some of our childrens died.\n");
                	break;
           		}
		   		speed += i;
		   		failed += j;
		   		bytes += k;
		   		/* fprintf(stderr, "*Knock* %d %d read=%d\n", speed, failed, pid); */
		   		if(--clients == 0) break;
	   		}
	   		fclose(f);

  	   		printf("\nSpeed=%d pages/min, %d bytes/sec.\nRequests: %d susceed, %d failed.\n",
	   		(int)((speed + failed) / (benchtime / 60.0f)),
	   		(int)(bytes / (float)benchtime),
	   		speed,
	   		failed);
  	}
  	return i;
}

static void benchcore(const char *host, const int port, const char *req)
{
 	int rlen;
 	char buf[1500];
 	int s, i;
 	struct sigaction sa;

 	/* setup alarm signal handler */
 	sa.sa_handler = alarm_handler;
 	sa.sa_flags = 0;
 	if(sigaction(SIGALRM, &sa, NULL))
    	exit(3);
 	alarm(benchtime);

	 rlen = strlen(req);
 	nexttry:while(1)		//I don't understand??????????????		
 	{
    	if(timerexpired)
    	{
       		if(failed > 0)
       		{
          		/* fprintf(stderr,"Correcting failed by signal\n"); */
          		failed--;
       		}
       		return;
    	}
    	s = Socket(host, port);                          
    	if(s < 0){ failed++; continue;} 
    	if(rlen != write(s, req, rlen)) {failed++; close(s); continue;}
    	if(http10==0) 
	    	if(shutdown(s, 1)) { failed++; close(s); continue;}
    	if(force == 0) 	//need read data from connected socket
    	{
            /* read all available data from socket */
	    	while(1)
	    	{
            	if(timerexpired) break; 
	      		i = read(s, buf, 1500);
              	/* fprintf(stderr,"%d\n",i); */
	        	if(i < 0) 
            	{	 
                 	failed++;
                 	close(s);
                 	goto nexttry;
            	}
	        	else
				{
		       		if(i == 0) 
						break;
		       		else
			       		bytes+=i;
				}	
	    	}
    	}
    	if(close(s)) {failed++; continue;}
    	speed++;
 	}
}
