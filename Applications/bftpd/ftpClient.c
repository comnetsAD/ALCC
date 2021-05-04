#include "FTPSnC.h"
#include "logMessages.h"

char lsBuf[1024]; //to hold ls command output.
char pwd[256];    //to hold pwd.
char serverIP[32];
char filename[64]; //holds the name of the file to put;

FILE *FTPfp;

/*Check if the command has to be handled on client side itself*/

unsigned int isClientSide(char *buf){
  if ((strncmp("!LS", buf, 3) && strncmp("!ls", buf, 3)) == 0){
    return 1;
  }
  if ((strncmp("!PWD", buf, 4) && strncmp("!pwd", buf, 4)) == 0){
    return 2;
  }
  if ((strncmp("!CD", buf, 3) && strncmp("!cd", buf, 3)) == 0){
    return 3;
  }
  if ((strncmp("PUT", buf, 3) && strncmp("put", buf, 3)) == 0){
    return 4;
  }
  if ((strncmp("EXIT", buf, 4) && strncmp("exit", buf, 4)) == 0){
    return 5;
  }
  return 0;
}

/*Extend this function to add the put functionality. */

void putFTP(char *filename){
  printf("\rServer: file successfully received\n");
}

/*This function handles all the GET command feature.
 * Basically, it connects to the newly created socket opened
 * by the FTP server to download the file. How does it know?
 * Well, that's how it's designed. Basically, the control function
 * which calls it knows if the server is willing to let the client
 * GET the file.
 */

void getFTP(char *filename){

  int bytesReceived;
  char recvBuff[256];
  printf("\rReady to receive file: %s\nSTATUS: OK\n\n", filename);
  struct sockaddr_in ftpaddr;
  int sockftp;
  if (( sockftp = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    printf("FTP Socket err!\n");
    return;
  }

  bzero(&ftpaddr, sizeof(ftpaddr));
  ftpaddr.sin_family = AF_INET;
  ftpaddr.sin_port = htons(21);
  if (inet_pton(AF_INET, serverIP, &ftpaddr.sin_addr) <= 0){
    fprintf(stderr, "Failed to match FTP IP\n");
    return;
  }

  if (connect(sockftp, (struct sockaddr *) &ftpaddr, sizeof(ftpaddr)) <0){
    fprintf(stderr, "Connection error\n");
  }
  else {


    FTPfp = fopen(filename, "w+");
    if(FTPfp == NULL){
      printf("Error opening file %s\n", filename);
      close(sockftp);
      return;
    }

    /* Reading file in chunk is not our original idea and has
     * been acknowledged in the server source code as well.
     */

    while((bytesReceived = read(sockftp, recvBuff, 256)) > 0){
      printf("%d bytes received\n",bytesReceived);
      fwrite(recvBuff, 1, bytesReceived, FTPfp);
    }

    if(bytesReceived < 0){
        printf("\rFile corrupted!!! Try again. \n");
    }
    fclose(FTPfp);
    printf("File received.\n");

  }
  close(sockftp);
  return;
}

/*
 *This is the entry function for commands like GET and PUT
 */

unsigned int handleFTP(char *buf){
  if (strncmp(buf, "FILEOK ", 7) == 0){
      getFTP(buf+7);
    }
  if (strncmp(buf, "FILERR", 6) == 0){
    printf("File Error on FTP Server side\n");
  }
  if (strncmp(buf, "PUTOK", 5) == 0){
    printf("Server: file successfully received\n");
    return 0;
  }
  if (strncmp(buf, "PUTERR",6) == 0){
    printf("FTP Server can't accept any files at the moment.\n");
  }
  return 1;
}

/*
 * The function which handles all the network request and response.
 * This is the core function which is always active once the client
 * is connected to the server.
 */
void str_cli(FILE *fp, int sockfd){

  int maxfdp1, stdineof;
  fd_set rset;
  char buf[MAXLINE];
  char stdbuf[MAXLINE];
  int n, commandCode;

  stdineof = 0;
  FD_ZERO(&rset);

  for (; ;){
    if (stdineof == 0){
      FD_SET(fileno(fp), &rset);
    }

    FD_SET(sockfd, &rset);
    maxfdp1 = max(fileno(fp), sockfd) + 1;
    select(maxfdp1, &rset, NULL, NULL, NULL);

    /* Socket ready; read from socket now */
    if (FD_ISSET(sockfd, &rset)){
      if ((n=read(sockfd, buf, MAXLINE)) == 0){
	if (stdineof == 1){
	  return;
	}
	else{
	  fprintf(stderr, "Connection Terminated\n");
	  exit(1);
	}
      }
      buf[n] = '\0';
      if (handleFTP(buf)){
	write(fileno(stdout), buf, n);
      }
    }

    /* Read from the stdin now */
    if (FD_ISSET(fileno(fp), &rset)){
      n = read(fileno(fp), stdbuf, MAXLINE);
      if (n  == 0){
	stdineof = 1;
	shutdown(sockfd, SHUT_WR);
	FD_CLR(fileno(fp), &rset);
	continue;
      }

      else{
	commandCode = isClientSide(stdbuf);
	char *temptr;

	 stdbuf[n-1] = (stdbuf[n-1] == '\n') ? '\0': stdbuf[strlen(stdbuf)-1];
	 stdbuf[n] = '\0';



	switch (commandCode){


	case 1:
	  getcwd(pwd, sizeof(pwd));
	  ls(pwd, lsBuf);
	  printf("<-------------FILE(client) LISTING-------------->\n%s\n",
		 lsBuf);
	  break;

	case 2:
	  getcwd(pwd, sizeof(pwd));
	  printf("%s", pwd);
	  break;

	case 3:

	  /*Cleaning the input for chdir*/
	  temptr = stdbuf+4;
	  while(*temptr){
	    if (*temptr == '\n'){
	      *temptr = '\0';
	      break;
	    }
	    temptr++;
	  }

	  if(chdir(stdbuf+4) < 0){
	    printf("%s:%s", CD_ERR, stdbuf+4);
	  }
	  else{
	    printf("%s", CD_SUCC);
	    getcwd(pwd, sizeof(pwd));
	    printf("%s", pwd);
	  }

	  break;

	case 4:

	  printf("Waiting on server to receive file %s\n", stdbuf+4);
	  FTPfp = fopen(stdbuf+4, "r");
	  if (FTPfp == NULL){
	    printf("Error reading file: %s\n", stdbuf+4);
	    break;
	  }
	  write(sockfd, stdbuf, strlen(stdbuf));
	  break;

	case 5:
	  close(sockfd);
	  return;
	}// switch ends here

        if (commandCode){
	  stdbuf[0] = '\0';
	  if (commandCode != 4){
	    printf(HLINE);
	  }
	}

	write(sockfd, stdbuf, n);
      }

    }
  }
}


int main(int argc, char **argv){
  int sockfd, n;
  char recvline[MAXLINE + 1];
  struct sockaddr_in servaddr;

  if (argc != 2){
    fprintf(stderr, "Usage: FTPClient <FTPServerIPaddress>.\n");
    exit(1);
  }

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    fprintf(stderr, "Socket Error!\n");
    exit(1);
  }

  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(21);

  if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0){
    fprintf(stderr, "%s couldn't recognize the entered IP addr %s as a valid IP", argv[0], argv[1]);
    exit(1);
  }

  if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) <0){
    fprintf(stderr, "Connection error\n");
  }
  strcpy(serverIP, argv[1]);
  str_cli(stdin, sockfd);

  exit(0);
}
