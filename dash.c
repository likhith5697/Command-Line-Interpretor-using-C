#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>

#define MAX_COMMANDS 2000
char error_message[30] = "An error has occurred\n"; //global error handling char array
 // Function to execute a command using execv()
void execute_command(const char *command) {
    //Re-direction logic => 
    char *output_redirect = NULL; //created to store the destination file name of output
    char *cmd_copy = strdup(command); //duplicates the command to avoid changing the original command
    
    //search for '> character in the input command
    char *redirection_char = strchr(cmd_copy,'>');
    int redirection_count = 0;
    char *tmp_redirection_char = redirection_char;
    // handling base cases
    if (strcmp(command, "cd") == 0){
		write(STDERR_FILENO, error_message, strlen(error_message));
//		exit(1);
    }

    if(strcmp(command,"exit bad")==0){
	write(STDERR_FILENO, error_message, strlen(error_message));
	return;
    }


    while (tmp_redirection_char != NULL) {
	     redirection_count++;
             tmp_redirection_char = strchr(tmp_redirection_char + 1, '>');
    }

      if (redirection_count > 1) {
              // Multiple '>' characters found, throw an error
	             write(STDERR_FILENO, error_message, strlen(error_message));
	             free(cmd_copy);
		     exit(1);
	            // return;
	    }
	      


    //it divides the re-direction command into two , (1) Executable command (2) Outputfile 
    if(redirection_char!=NULL){
	*redirection_char = '\0'; //replace it with null terminator
	output_redirect = redirection_char +1; // stores the file name exactly after '>' character

	//skip the leading, trailing spaces in the output file name
	while(*output_redirect ==' ' || *output_redirect == '\t'){
		output_redirect++;
	}
	int length = strlen(output_redirect);
	while(length>0 && (output_redirect[length-1]==' '|| output_redirect[length-1]=='\t')){
		output_redirect[length-1]='\0';
		length--;
	}
    }

    if(strlen(cmd_copy)==0){
	 write(STDERR_FILENO, error_message, strlen(error_message));
	 free(cmd_copy);
	 return;
    }


    pid_t child_pid = fork();
    if (child_pid == 0) { //child process
	if(output_redirect!=NULL){ 
	   int output_fd = open(output_redirect,O_WRONLY | O_CREAT | O_TRUNC,0666); //Creates a new file for output, if already exists, just rewrites it
	   if(output_fd == -1){ //if no file exists
	     // perror("open");
	      write(STDERR_FILENO, error_message, strlen(error_message)); //throws error
	      exit(1);
	   }
	   if(dup2(output_fd,STDOUT_FILENO)==-1 || dup2(output_fd,STDERR_FILENO)==-1){   //if duplicate copy fails, throw error
	 //  	perror("dup2");
		write(STDERR_FILENO, error_message, strlen(error_message));
		exit(1);
	   }
	   close(output_fd);
	}

	//new added code
	


// end 
        char *cmd_args[] = { "/bin/sh", "-c", (char *)command, NULL }; //path of shell, c refers that next string will be treated as a command
        execv("/bin/sh", cmd_args);
	write(STDERR_FILENO, error_message, strlen(error_message)); //new change
        //perror("exec");
        //	fprintf(stderr, "An error has occurred\n");
	//write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    } else if (child_pid < 0) {
        //perror("fork");
	write(STDERR_FILENO, error_message, strlen(error_message));
    } else {  //parent process
        int status;
        waitpid(child_pid, &status, 0);
    }
}











// Function to parse and execute user input
void parse_input(const char *input) {
    //check whether user wants to change the current directory
    if (strncmp(input, "cd ", 3) == 0) {
    //stores new destination directory present after "cd "
        const char *new_dir = input + 3;
        if (strlen(new_dir) == 0){
	   write(STDERR_FILENO, error_message, strlen(error_message)); 
	}
	//if directory is not changed, throw an error
	else{

	if(chdir(new_dir)!=0){
		 write(STDERR_FILENO, error_message, strlen(error_message));
	}
//	    write(STDERR_FILENO, error_message, strlen(error_message));
 //           perror("chdir");
        }
    }

    else if(strcmp(input,"&")==0){
	write(STDERR_FILENO, error_message, strlen(error_message));
    }


    else {
        char *commands[MAX_COMMANDS];
        int num_commands = 0;
        // Tokenize the input by "&"
        char *token = strtok((char *)input, "&");
        while (token != NULL && num_commands < MAX_COMMANDS) {
            commands[num_commands++] = token;
            token = strtok(NULL, "&");
        }
        // Execute each command in parallel
	int i;
        for (i = 0; i < num_commands; i++) {
            char *cmd = commands[i];
            // Remove leading and trailing spaces
            while (*cmd == ' ' || *cmd == '\t') {
                cmd++;
            }
            int len = strlen(cmd);
            while (len > 0 && (cmd[len - 1] == ' ' || cmd[len - 1] == '\t')) {
                cmd[len - 1] = '\0';
                len--;
            }
	    //call execute commands function
            if (len > 0) {
                execute_command(cmd);
            }
        }
    }
}

//Execute Function for Batch Mode
void execute_commands_from_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    //If file does not exist / does not open, throw an error
    if (file == NULL) {
        write(STDERR_FILENO, error_message, strlen(error_message));
 //       perror("fopen");
 	  exit(1);
        //return;
    }
    char *line = NULL;
    size_t len = 0;
    int is_empty = 1;

    //Read input and store the input lines of file in line till the end of file
    while (getline(&line, &len, file) != -1) {
        size_t line_length = strlen(line);
        if (line_length > 0 && line[line_length - 1] == '\n') { //if new line character is found at end of string
            line[line_length - 1] = '\0'; //replacing it with null terminator
        }

//	if(strcmp(line,"cd")==0){
//	 write(STDERR_FILENO, error_message, strlen(error_message));
//	}
	//call the parse function to parse the commands
        parse_input(line);
	is_empty =0;
    }
    if (line) { //checks if the memory is allocated or not after reading input using getline
        free(line); //de-allocates memory 
    }
    //close the file
    fclose(file);

    if (is_empty) {
          write(STDERR_FILENO, error_message, strlen(error_message));
     }
}



int main(int argc, char *argv[]) {
//if argument count is 2 means , it is batch mode, so batch mode function is called
     if (argc == 2) {
           execute_commands_from_file(argv[1]);
	   exit(1);
	       } 
// Throws an error if argument count exceeds 2
     else if (argc > 2) {
            write(STDERR_FILENO, error_message, strlen(error_message));
//	    fprintf(stderr, "Usage: %s [batch_file]\n", argv[0]);
	    exit(1);
	}
	//Prints dash prompt (interactive mode)
    while (1) {
        char *line = NULL;
        size_t len = 0;

        printf("dash> "); // Print the prompt here
        fflush(stdout);

        ssize_t read = getline(&line, &len, stdin); //reads input 
        if (read == -1) { 
            break; // Exit on EOF
        }
	if(strncmp(line,"exit",4)==0){
	    break; //break from the interactive mode if user enters exit
	}
        size_t line_length = strlen(line);
        if (line_length > 0 && line[line_length - 1] == '\n') {    //removes the newline character from the end of the string
            line[line_length - 1] = '\0'; //replaces with a null terminating character to make the string a valid one
        }

        parse_input(line); //calls the parse function
    }

    return 0;
}
