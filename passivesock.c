/* passivesock.c - passivesock */
/*
 * This code was adapted from the Stevens and 
 * the Comer books on Network Programming
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <netinet/in.h>

#include <netdb.h>


static u_short	portbase = 37000;    /* port base, for non-root servers	*/

/*------------------------------------------------------------------------
 * passivesock - allocate & bind a server socket using TCP or UDP
 *------------------------------------------------------------------------
 */

 //this file listens for connections only
int
passivesock( 
	char	*service,   /* service associated with the desired port	*/
	char	*protocol,  /* name of protocol to use ("tcp" or "udp")	*/
	int	qlen,	    /* max length of the server request queue	*/
	int	*rport )
{
	struct servent	*pse;	/* pointer to service information entry	*/
	struct protoent *ppe;	/* pointer to protocol information entry*/
	struct sockaddr_in sin;	/* an Internet endpoint address		*/
	int	s, type;	/* socket descriptor and socket type	*/

	//initialize sockaddr_in structure
	memset((char *)&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET; //set for ipv4
	sin.sin_addr.s_addr = INADDR_ANY; //set to accept any address

    /* Map service name to port number */
	if ( *rport )
	{
		/*	If a 1 is in this field, let OS choose a free port */
		sin.sin_port = htons((u_short) 0);
	}
	else
	{
		// else look up the service name (get service by name) 
		//and if it fails try to convert service name to a port number
		if ( pse = getservbyname(service, protocol) )
			sin.sin_port = htons(ntohs((u_short)pse->s_port)
				+ portbase);
		else if ( (sin.sin_port = htons((u_short)atoi(service))) == 0 )
		{
			fprintf( stderr, "can't get \"%s\" service entry\n", service);
			exit(-1);
		}
	}

    /* Map protocol name to protocol number  get proto by name
		tcp = 6, udp = 17 */
	if ( (ppe = getprotobyname(protocol)) == 0)// ppe = protocol entry/number
	{
		fprintf( stderr, "can't get \"%s\" protocol entry\n", protocol);
		exit(-1);
	}

    /* Use protocol to choose a socket type */
	if (strcmp(protocol, "udp") == 0)
		type = SOCK_DGRAM; //set socketType = UDP
	else
		type = SOCK_STREAM; //set socketType = TCP

    /* Allocate/Create the socket */
	s = socket(PF_INET, type, ppe->p_proto); //
	//Domain =PF_INET ==> IPv4
	//type = SOCK_STREAM ==> TCP or UDP; How data is transmitted
	//protocol = ppe->p_proto ==> protocol number; Specified actual protocol
	if (s < 0)
	{
		fprintf( stderr, "can't create socket: %s\n", strerror(errno));
		exit(-1);
	}

    /* Bind the socket to a Port/local Address (sin) */
	if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		fprintf( stderr, "can't bind to %s port: %s\n", service, strerror(errno));
		exit(-1);
	}
	//Listen for connections TCP only(SOCK_STREAM) 
	//qlen = max connections that can wait in queue
	if (type == SOCK_STREAM && listen(s, qlen) < 0)
	{
		fprintf( stderr, "can't listen on %s port: %s\n", service, strerror(errno));
		exit(-1);
	}

	if ( *rport ) // if the port (rport) is not 0 (you ask for a free port)
	{
		int	len;

		/* return the selected port by OS in rport */
		len = sizeof(sin);
		//getsockname fills sin struct with addr of socket and port
		//that is bound to the socket
		if ( getsockname( s, (struct sockaddr *)&sin, &len ) )
		{
			fprintf(  stderr, "chatd: cannot getsockname: %s\n", strerror(errno) );
			exit(-1);
		}
		//rport stores sockets port number saved in siin
		*rport = ntohs(sin.sin_port);	

	}
	//return socket descriptor
	return s;
}
