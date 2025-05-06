#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <pthread.h>
#include "passivesock.c"

#define	QLEN			5
#define	BUFSIZE			4096

int passivesock( char *service, char *protocol, int qlen, int *rport );

void *echo( void *s )
{
	//newly threaded socket to client
	char buf[BUFSIZE];
	int cc;
	int ssock = *(int*) s;

	/* start working for this guy */
	/* ECHO what the client says */
	for (;;)
	{
		if ( (cc = read( ssock, buf, BUFSIZE )) <= 0 )
		{
			printf( "The client has gone.\n" );
			close(ssock);
			break;
		}
		else
		{
			buf[cc] = '\0';
			//printf( "The client says: %s\n", buf );
			if ( write( ssock, buf, cc ) < 0 )
			{
				/* This guy is dead */
				close( ssock );
				break;
			}
		}
	}
	//if client disconnects or error occurs close the socket and exit thread
	pthread_exit(NULL);
}


/*
*/
int
main( int argc, char *argv[] )
{
	char			*service;
	struct sockaddr_in	fsin;
	int			alen = sizeof(fsin);
	int			msock;
	int			ssock;
	int			rport = 0;
	
	switch (argc) 
	{
		case	1:
			// No args? let the OS choose a port and tell the user
			rport = 1;
			break;
		case	2:
			// User provides a port? then use it
			service = argv[1];
			break;
		default:
			fprintf( stderr, "usage: server [port]\n" );
			exit(-1);
	}

	// Call the function that sets main socket to listen for connections
	int **clients_score = malloc (sizeof(int*) * 1024); // max num of sockets possible to make so each socket has an index to a score
	msock = passivesock( service, "tcp", QLEN, &rport );

	if (rport)
	{
		// Tell the user the selected port number
		printf( "server: port %d\n", rport );	
		fflush( stdout );
	}

	
	for (;;)
	{
		int	ssock;
		pthread_t	thr;

		ssock = accept( msock, (struct sockaddr *)&fsin, &alen );
		if (ssock < 0)
		{
			fprintf( stderr, "accept: %s\n", strerror(errno) );
			break;
		}
		int *psock = malloc(sizeof(int));
		*psock = ssock;

		printf( "A client has arrived for echoes - serving on fd %d.\n", ssock );
		fflush( stdout );

		//creates new thread and sends it the socket to echo function
		pthread_create( &thr, NULL, echo, psock);

	}
	pthread_exit(NULL);
}


