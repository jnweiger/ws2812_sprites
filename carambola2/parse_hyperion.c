/*
 * parse_hyperion.c 
 * receive commands from the android hyperion app
 *
 * 2014 (c) juewei@fabfolk.com
 * Apply GPLv2.0 or ask.
 */

FIXME: the read blocks us after the first success reply.

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>	// exit()
#include <strings.h>	// bzero()

#define PORT 19444

int parse_hyperion(char *p, int len)
{
  int r=-1, g=-1, b=-1;

  write(2, p, len); write(2, "\n\r", 2);

  while (len-- > 0 && (*p < '0' || *p > '9')) p++;
  if    (len-- > 0) r = *p - '0';
  while (len-- > 0 && (*p >= '0' && *p <= '9')) r = 10 * r + (*p - '0');
  if    (len--<= 0 || *p != ',') return 0;
  p++;
  if    (len-- > 0) g = *p - '0';
  while (len-- > 0 && (*p >= '0' && *p <= '9')) g = 10 * g + (*p - '0');
  if    (len--<= 0 || *p != ',') return 0;
  p++;
  if    (len-- > 0) b = *p - '0';
  while (len-- > 0 && (*p >= '0' && *p <= '9')) b = 10 * b + (*p - '0');

  if (b < 0) return 0;
  printf("color: %d,%d,%d\n", r,g,b);
  return 1;
}

int
main(int ac, char **av)
{
  char buf[256];
  struct sockaddr_in addr, peer;
  int  n;
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sockfd < 0)
    { 
      perror("server socket failed");
      exit(1);
    }

  printf("socket=%d\n", sockfd);

  /* Initialize socket structure */
  bzero((char *) &addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(PORT);
 

  int yes = 1;
  if ( setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1 )
    {
      perror("setsockopt");
    }


  /* Now bind the host address using bind() call.*/
  if (bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0)
    {
      perror("bind failed");
      exit(2);
    }

  /* Now start listening for the clients, here process will
   * go in sleep mode and will wait for the incoming connection
   */
  listen(sockfd, 5);

  for (;;)
    {
      int peer_len = sizeof(peer);

      /* Accept actual connection from the client */
      int fd = accept(sockfd, (struct sockaddr *)&peer, &peer_len);
      if (fd < 0) 
        {
          perror("accept failed.");
          exit(3);
        }

      /* start communicating */
      n = read(fd, buf, sizeof(buf) );
      if (n <= 0) 
        {
	  continue;
        }

      parse_hyperion(buf, n);

      /* Write a response to the client */
#     define reply "{\"success\": true}\n"
      n = write(fd,reply, sizeof(reply));
      if (n < 0)
        {
          perror("reply failed.");
          exit(5);
        }
    }
  return 0; 
}
