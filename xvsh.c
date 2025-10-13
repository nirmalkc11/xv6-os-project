#include "types.h"
#include "fcntl.h"
#include "user.h"

#define SH_PROMPT "xvsh> "
#define NULL (void *)0


char *strtok(char *s, const char *delim);

int process_one_cmd(char *);

#define MAXLINE 256

int main(int argc, char *argv[])
{
    char buf[MAXLINE];
    int i;
    int n;

    while (1)
    {
        printf(1, SH_PROMPT);  // print the prompt before each command

        n = read(0, buf, MAXLINE);
        if (n == 0)
            break;  // EOF -> exit shell

        if (n == 1)  // only newline entered
            continue;

        buf[i = (strlen(buf) - 1)] = 0;  // replace newline with null

        process_one_cmd(buf);

        // Wait for all foreground children to finish
        while (wait() > 0)
            ;

        memset(buf, 0, sizeof(buf));
    }

    exit();
}


int exit_check(char **tok, int num_tok)
{
    // your implementation here
    if (tok[0] == 0)
        return 0;

    // Check if the first token is "exit"
    if (strcmp(tok[0], "exit") == 0)
        return 1;
    return 0;
}

int process_normal(char **tok, int bg)
{
    int pid = fork();

    if (pid < 0) {
        printf(1, "fork failed\n");
        return -1;
    }

    if (pid == 0) {
        // Child runs the command
        if (exec(tok[0], tok) < 0) {
            printf(1, "Cannot run this command %s\n", tok[0] ? tok[0] : "");
            exit();
        }
        // not reached
    } else {
        // Parent (xvsh)
        if (bg) {
            // Background: do NOT wait; just report pid
            printf(1, "[pid %d] runs as a background process\n", pid);
            printf(1, "xvsh> "); // do not call wait() here
        } else {
            // Foreground: wait for child to finish
            wait();
        }
    }

    return 0;
}


int process_one_cmd(char* buf)
{
    int i, num_tok;
    char **tok;
    int bg;
    i = (strlen(buf) - 1);
    num_tok = 1;

    while (i)
    {
        if (buf[i--] == ' ')
            num_tok++;
    }

    if (!(tok = malloc( (num_tok + 1) *   sizeof (char *)))) 
    {
        printf(1, "malloc failed\n");
        exit();
    }        


    i = bg = 0;
    tok[i++] = strtok(buf, " ");

    
     /* check special symbols (background &) */
  bg = 0;
  while ((tok[i] = strtok(NULL, " ")))
  {
    if (strcmp(tok[i], "&") == 0) {
        bg = 1;
        tok[i] = 0;   // terminate argv so exec doesn't see "&"
        break;        // ignore anything after '&' (simple shell)
     }
     i++;
   }

// ensure argv is null-terminated if no '&'
tok[i] = 0;


    /*Check buid-in exit command */
    if (exit_check(tok, num_tok))
    {
        
       exit();
        
    }

    // your code to check NOT implemented cases
    /*Check built-in exit command */
if (exit_check(tok, num_tok))
{
    // Wait for all children (including background ones)
    while (wait() > 0)
        ;

    exit();
}


// Check for pipe '|'
int pipe_index = -1;
for (i = 0; tok[i]; i++) {
    if (strcmp(tok[i], "|") == 0) {
        pipe_index = i;
        break;
    }
}

if (pipe_index != -1) {
    // Split the tokens into left and right commands
    tok[pipe_index] = 0; // break the command into two parts
    char **left_cmd = tok;
    char **right_cmd = &tok[pipe_index + 1];

    int p[2];
    pipe(p);

    int pid1 = fork();
    if (pid1 == 0) {
        // Child 1: left command (write side)
        close(1);       // close stdout
        dup(p[1]);      // replace stdout with write end
        close(p[0]);
        close(p[1]);
        if (exec(left_cmd[0], left_cmd) < 0) {
            printf(1, "Cannot run this command %s\n", left_cmd[0]);
            exit();
        }
    }

    int pid2 = fork();
    if (pid2 == 0) {
        // Child 2: right command (read side)
        close(0);       // close stdin
        dup(p[0]);      // replace stdin with read end
        close(p[0]);
        close(p[1]);
        if (exec(right_cmd[0], right_cmd) < 0) {
            printf(1, "Cannot run this command %s\n", right_cmd[0]);
            exit();
        }
    }

    // Parent process
    close(p[0]);
    close(p[1]);
    wait();
    wait();

    free(tok);
    return 0;
}


// Check for output redirection '>'
int redir_index = -1;
for (i = 0; tok[i]; i++) {
    if (strcmp(tok[i], ">") == 0) {
        redir_index = i;
        break;
    }
}

if (redir_index != -1) {
    // Separate command and filename
    tok[redir_index] = 0;
    char *outfile = tok[redir_index + 1];

    if (outfile == 0) {
        printf(1, "No output file specified\n");
        free(tok);
        return 0;
    }

    int pid = fork();
    if (pid == 0) {
        // Child: redirect stdout to file
        int fd = open(outfile, O_CREATE | O_WRONLY);
        if (fd < 0) {
            printf(1, "Cannot open %s\n", outfile);
            exit();
        }
        close(1);   // close stdout
        dup(fd);    // duplicate file descriptor to stdout
        close(fd);  // close original fd

        if (exec(tok[0], tok) < 0) {
            printf(1, "Cannot run this command %s\n", tok[0]);
            exit();
        }
    } else {
        wait();  // parent waits for child
    }

    free(tok);
    return 0;
}

// If no pipe or redirection, handle normal command
process_normal(tok, bg);

if (!bg)
    printf(1, SH_PROMPT);

free(tok);
return 0;
}



char *
strtok(s, delim)
    register char *s;
    register const char *delim;
{
    register char *spanp;
    register int c, sc;
    char *tok;
    static char *last;


    if (s == NULL && (s = last) == NULL)
        return (NULL);

    /*
     * Skip (span) leading delimiters (s += strspn(s, delim), sort of).
     */
cont:
    c = *s++;
    for (spanp = (char *)delim; (sc = *spanp++) != 0;) {
        if (c == sc)
            goto cont;
    }

    if (c == 0) {        /* no non-delimiter characters */
        last = NULL;
        return (NULL);
    }
    tok = s - 1;

    /* I got this part from stackoverflow
     * Scan token (scan for delimiters: s += strcspn(s, delim), sort of).
     * Note that delim must have one NUL; we stop if we see that, too.
     */
    for (;;) {
        c = *s++;
        spanp = (char *)delim;
        do {
            if ((sc = *spanp++) == c) {
                if (c == 0)
                    s = NULL;
                else
                    s[-1] = 0;
                last = s;
                return (tok);
            }
        } while (sc != 0);
    }
    /* NOTREACHED */
}

