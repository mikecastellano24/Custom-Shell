// Michael Castellano - Building Custom Shell

#define _POSIX_C_SOURCE 200809L // required for strdup() on cslab
#define _DEFAULT_SOURCE // required for strsep() on cslab
#define _BSD_SOURCE // required for strsep() on cslab

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#define MAX_ARGS 32

// Helper function which checks the conditions for redirecting I/O and 
// redirects if conditions hold
void redirect_io(char **args, size_t num_args)
{
    // Count number of redirects we have
    int input_redir_count = 0;
    int output_redir_count = 0;

    // Iterate through our arguments to check for redirects
    for (size_t i = 0; i < num_args; i++)
    {   
        // Do we have any redirects?
        if (strcmp(args[i], "<") == 0 || strcmp(args[i], ">") == 0 || strcmp(args[i], ">>") == 0)
        {
            int fd;
            // Owner flags as a precaution (not necessary)
            int owner_flags = S_IRUSR | S_IWUSR;

            // Check if redirect is '<'
            if (strcmp(args[i], "<") == 0)
            {
                if (++input_redir_count > 1)
                {
                    perror("Can't have two <'s!\n");
                    exit(1);
                }

                fd = open(args[i+1], O_RDONLY, owner_flags);
            }
            // Check if redirct is '>'
            else if (strcmp(args[i], ">") == 0)
            {            
                if (++output_redir_count > 1)
                {
                    perror("Can't have two >'s or >>'s!\n");
                    exit(1);
                }
            
                fd = open(args[i+1], O_RDWR | O_CREAT, owner_flags);
            }
            // Redirect must be '>>'
            else 
            {
                if (++output_redir_count > 1)
                {
                    perror("Can't have two >'s or >>'s!\n");
                    exit(1);
                }
                
                // Rd/wr, create, and append mode for output redirection
                fd = open(args[i+1], O_RDWR | O_CREAT | O_APPEND, owner_flags);
            }

            // Null check file
            if (fd == -1)
            {
                perror("Open");
                exit(1);
            }           

            // Redirect I/O based on current operator
            if (strcmp(args[i], "<") == 0)
            {
                dup2(fd, STDIN_FILENO);
            }
            // > or >>
            else 
            {
                dup2(fd, STDOUT_FILENO);
            }
             

            close(fd);

            // Removes operator from args
            args[i] = NULL;
        }
    }
}



char **get_next_command(size_t *num_args)
{
    // print the prompt
    printf("cssh$ ");

    // get the next line of input
    char *line = NULL;
    size_t len = 0;
    getline(&line, &len, stdin);
    if (ferror(stdin))
    {
        perror("getline");
        exit(1);
    }
    if (feof(stdin))
    {
        return NULL;
    }

    // turn the line into an array of words
    char **words = (char **)malloc(MAX_ARGS*sizeof(char *));
    int i=0;

    char *parse = line;
    while (parse != NULL)
    {
        char *word = strsep(&parse, " \t\r\f\n");
        if (strlen(word) != 0)
        {
            words[i++] = strdup(word);
        }
    }
    *num_args = i;
    for (; i<MAX_ARGS; ++i)
    {
        words[i] = NULL;
    }

    // all the words are in the array now, so free the original line
    free(line);

    return words;
}

void free_command(char **words)
{
    for (int i=0; i<MAX_ARGS; ++i)
    {
        if (words[i] == NULL)
        {
            break;
        }
        free(words[i]);
    }
    free(words);
}

int main()
{
    size_t num_args;

    // get the next command
    char **command_line_words = get_next_command(&num_args);
    while (command_line_words != NULL)
    {
        // run the command here
        // don't forget to skip blank commands
        if (num_args > 0)
        {
            // and add something to stop the loop if the user
            // runs "exit"
            if (strcmp(command_line_words[0], "exit") == 0)
            {
                break;
            }

            pid_t pid = fork();
            if (pid == -1)
            {
                perror("fork");
                exit(1);
            }
         
            // Child process
            if (pid == 0)
            {
                redirect_io(command_line_words, num_args);
                if (execvp(command_line_words[0], command_line_words) == -1)
                {
                    perror("execvp");
                    exit(1);
                }       
            }   
            // Parent process
            else 
            {
                if (waitpid(pid, NULL, 0) == -1)
                {
                    perror("waitpid");
                    exit(1);
                }
            }
        }

        // free the memory for this command
        free_command(command_line_words);

        // get the next command
        command_line_words = get_next_command(&num_args);
    }

    // free the memory for the last command
    free_command(command_line_words);

    return 0;
}
