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
    printf(1, SH_PROMPT);  /* print prompt (printf requires %% to print %) */

    while ( (n = read(0, buf, MAXLINE)) != 0) 
    {
        if (n == 1)                           /* no input at all, we should skip */
        {
            printf(1, SH_PROMPT);
            continue;
        }
        buf[i = (strlen(buf) - 1)] = 0;       /* replace newline with null */

        process_one_cmd(buf);
        
        printf(1, SH_PROMPT);
      
        memset(buf, 0, sizeof(buf));
    }
    
    exit();
}

int exit_check(char **tok, int num_tok)
{
    // your implementation here
    return 0;
}

int process_normal(char **tok, int bg)
{
    // your implementation here
    // note that exec(*tok, tok) is the right way to invoke exec in xv6
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

    /* check special symbols */
    while ((tok[i] = strtok(NULL, " "))) 
    {
        switch (*tok[i]) 
        {
            case '&':
                bg = i;
                tok[i] = NULL;
                break;

            default:
                // do nothing
                break;
        }   
        i++;
    }

    /*Check buid-in exit command */
    if (exit_check(tok, num_tok))
    {
        /*some code here to wait till all children exit() before exit*/
	// your implementation here
        exit();
    }

    // your code to check NOT implemented cases
    
    /* to process one command */
    process_normal(tok, bg);

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

    /*
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

