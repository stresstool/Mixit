/* QA.H -- header for using the QA Question & Answer routine(s) */

extern int _qaval_; /* global cell for (guaranteed) returned value */
extern int _qacnt_; /* global cell for number of characters read */

#define qa( prompt, bufsiz, buf ) qa5( stdin, stdout, prompt, bufsiz, buf )

/* error recovery for QA and QA5; flush the input buffer */
#define purge_qa() purge_qa2( stdin, stdout )

extern void purgeHistory(void);

