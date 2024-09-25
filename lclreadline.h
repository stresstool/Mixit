#ifndef _LCLREADLINE_H_
#define _LCLREADLINE_H_ 1

extern int lclReadLine(void *fin, const char *prompt, char *buff, int buflen);
extern void lclPurgeReadLineHistory(void);

#endif	/* _LCLREADLINE_H_ */
