#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>


int main(int argc, char* argv[]){
   char* cmd;
   char* cmdArgs[20];
   int cmdArgsI = 0;
   int cmdPart = 0;
   int stdout;
   int stderr; 
   int stdOutUsed = 0;
   int stdErrUsed = 0;
   int timeoutUsed = 0;
   int timeout;
   int sleepUsed = 0;
   int sleepTime;
   int terminated = 0;
   int terminateVal;
   int usingRecentPath = 0;
   int segFault = 0;
   int p8First = 0;
   
   
   int i;
   for(i = 0; i < argc; i++){
       if(strcmp(argv[i], "./p8")==0 && p8First == 0){
	   p8First = 1;
       }
       else if(strcmp(argv[i], "./capture")==0 || strcmp(argv[i], "capture")==0){   
           char* temp1 = malloc(10);
           char* temp2 = malloc(10);     
           strcpy(temp1, argv[i+1]);
           strcpy(temp2, argv[i+1]); 
           char* stdoutFileName = strcat(temp1, ".stdout");
           char* stderrFileName = strcat(temp2, ".stderr");        
           stdout = open(stdoutFileName, O_CREAT | O_WRONLY, 0640);
           stderr = open(stderrFileName, O_CREAT | O_WRONLY, 0640);
           stdOutUsed = 1;
           stdErrUsed = 1;
           i++;
       }
       else if(strcmp(argv[i], "-o") == 0){
           char* temp1 = malloc(10);
           stdout = open(argv[i+1], O_CREAT | O_WRONLY, 0640);
           stdOutUsed = 1;
           i++;
       } 
       else if(strcmp(argv[i], "-e") == 0){
           char* temp1 = malloc(10);
           stderr = open(argv[i+1], O_CREAT | O_WRONLY, 0640);
           stdErrUsed = 1;
           i++;
       } 
       else if(strcmp(argv[i], "./to") == 0 || strcmp(argv[i], "to") == 0){
           timeout = atoi(argv[i+1]);
           timeoutUsed = 1;
           i++;
       } 
       else if(strcmp(argv[i], "-t") == 0){
           timeout = atoi(argv[i+1]);
           timeoutUsed = 1;
           i++;
       } 
       else if(strcmp(argv[i], "sleep") == 0){
           sleepTime = atoi(argv[i+1]);
           sleepUsed = 1;
           i++;
       }  
       else if(strcmp(argv[i], "-f") == 0){
           terminateVal = argv[i+1];
           terminated = 1;
           return atoi(argv[i+1]);
       } 
       else if(strcmp(argv[i], "-s") == 0){
           segFault = 1; 
       }
       else if(cmdPart){
           cmdArgs[cmdArgsI] = argv[i];
           cmdArgsI++;
       }
       else if(strcmp(argv[i], "-s") == 0){
           segFault = 1; 
       }
       else{
	       cmd = argv[i];
           cmdArgs[0] = cmd;
           cmdArgsI++;	
           cmdPart = 1;
       }
   }


   pid_t wpid;
   int status; //dup
   pid_t pid;

   pid = fork();
   if(pid == 0){
       if(stdOutUsed){
	  dup2(stdout, 1);
       }
       if(stdErrUsed){
          dup2(stderr,2);
       }
       char* newCmdArgs[cmdArgsI+1];
       int curr = 0;      
       while(curr < cmdArgsI){
           newCmdArgs[curr] = cmdArgs[curr];
           curr++;
       }
       newCmdArgs[curr] = (char*)0;
       execvp(newCmdArgs[0], newCmdArgs);
   }
   else if(pid < 0){
      return SIGTERM;
   }
   else{
    wait(status); 
	if(segFault){ 
	 return 11;
	}
      if(timeoutUsed && sleepUsed){
		  int currTime = timeout;
		  int currSleepTime = timeout;
		  if(timeout < sleepTime){
	         return SIGTERM;
          }  
		  else{
			  while(currSleepTime){
			      currSleepTime = sleep(currSleepTime); 
		      }
		  }
      }
      else if(sleepUsed){
		  while(sleepTime){
			  sleepTime = sleep(sleepTime);  
	      } 
	  }
   }

   return 0;
}

