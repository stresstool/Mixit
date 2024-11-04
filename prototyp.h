
/* asm.c */
extern int	GetRec_asm(InRecord *rec);
extern int	PutHead_asm(FILE *file, ulong addr, ulong hi);
extern int	PutRec_asm(FILE *file, uchar *data, int recsize, ulong recstart);

/* dio.c */
extern int	GetRec_dio(InRecord *rec);
extern int	PutRec_dio(FILE *file, uchar *data, int recsize, ulong recstart);
extern int	PutHead_dio(FILE *file, ulong addr, ulong hi);
extern int	PutFoot_dio(FILE *file);

/* cpe.c */
extern int	GetRec_cpe(InRecord *rec);
extern int	PutRec_cpe(FILE *file, uchar *data, int recsize, ulong recstart);
extern int	PutHead_cpe(FILE *file, ulong addr, ulong hi);
extern int	PutFoot_cpe(FILE *file);
extern int	PutXfer_cpe(FILE *file, ulong addr);

/* dld.c */
extern int	GetRec_dld(InRecord *rec);
extern int	PutRec_dld(FILE *file, uchar *data, int recsize, ulong recstart);
extern int	PutFoot_dld(FILE *file);

/* coff.c */
extern int	GetRec_coff(GPF *gpf, InRecord *rec, LogicalAddr lo, LogicalAddr hi, long reloaction );

#if INCLUDE_ELF
/* elf.c */
extern int	GetRec_elf(GPF *gpf, InRecord *rec, LogicalAddr lo, LogicalAddr hi, long relocation );
#endif

/* getfile.c */
extern void	save_data(InRecord *rec, LogicalAddr userLo, LogicalAddr userHigh, long relocation, GPF *gpf);
extern int	getfile(char *fname, GPF *gpf);

/* getform.c */
extern void	fileExtension(char *dest, char *filename, int maxlen);
extern FileFormat	getFormat(char *filename);

/* hexutl.c */
extern int	strtobytes(uchar *str, int nbytes);
extern int	strtohex(uchar *str, int nchars);
extern void	hextobytes(uchar *hexstr, int nbytes);
extern LogicalAddr	bytestoaddr(uchar *bytestr, int nbytes);
extern LogicalAddr	hextoaddr(uchar *hexstr, int nnybs);

/* image.c */
extern void	imageInit(Image *image, int pagesize);
extern void	imageFree(Image *image);
extern void	imageWrite(Image *image, LogicalAddr addr, size_t nbytes, uchar *data);
extern void	imageRead(Image *image, LogicalAddr addr, size_t nbytes, uchar *data);
extern void	symbolWrite(Image *image, uchar *sym, int len);
extern int	init_reader(GPF *gpf);
extern int	readImage(GPF *gpf, uchar *bufferPtr, ulong bufferSpace, ulong low_address, 
				ulong high_address, ulong *new_low, int *bytesRead);
extern void	imageDump(Image *image, FILE *dump);
extern int	imageCheck(Image *image, FILE *log);

/* img.c */
extern int	GetRec_img(InRecord *rec);
extern int	PutRec_img(FILE *file, uchar *data, int recsize, ulong recstart);

/* intel.c */
extern int	GetRec_intel(InRecord *rec);
extern int	PutFoot_intel(FILE *file);
extern int	PutRec_intel(FILE *file, uchar *data, int recsize, ulong recstart);

/* lda.c */
extern int	GetRec_lda(InRecord *record);
extern int	PutRec_lda(FILE *file, uchar *data, int recsize, ulong recstart);
extern int	PutFoot_lda(FILE *file);

/* mac.c */
extern int	macGetRec(InRecord *rec);
extern int	PutHead_mac(FILE *file, ulong addr, ulong hi);
extern int	PutRec_mac(FILE *file, uchar *data, int recsize, ulong recstart);

/* mot.c */
extern char	*to_hex(uchar value, char *str);
extern int	GetRec_mot(InRecord *rec);
extern int	PutFoot_mot(FILE *file);
extern int	PutRec_mot(FILE *file, uchar *data, int recsize, ulong recstart);

/* parser.c */
extern char	*sig(char *text);
extern int	showbad(int before, char *why);
extern int	ioparsebad(int input_parse, GPF *gpfp);

/* port2.c */
extern int	subscanf(char *source, char *format, char *out);
extern int	eprintf(const char *format, ...);
extern int	eprintl(const char *format, ...);

/* putfile.c */
extern int	putfile(char *fname, GPF *gpf);

/* qa.c */
extern int	qa5(FILE *fin, FILE *fout, char *prompt, int bufsiz, char *buf);
extern void	purge_qa2(FILE *fin, FILE *fout);

/* rom.c */
extern int	GetRec_rom(InRecord *rec);
extern int	PutRec_rom(FILE *file, uchar *data, int recsize, ulong recstart);
extern int	PutHead_rom(FILE *file, ulong addr, ulong hi);

/* tekhex.c */
extern int	GetRec_tekhex(InRecord *rec);
extern int	PutFoot_tekhex(FILE *file);
extern int	PutSym_tekhex(FILE *file, uchar *data, int recsize);
extern int	PutRec_tekhex(FILE *file, uchar *data, int recsize, ulong recstart);

/* varargs.c */
extern void	err_exit(char *fmt, ...) __attribute__ ((__format__ (__printf__, 1, 0)));
extern void	ev_exit(int eval, char *fmt, ...) __attribute__ ((__format__ (__printf__, 2, 0)));
extern void	perr_exit(char *fmt, ...) __attribute__ ((__format__ (__printf__, 1, 0)));
extern int	err_return(int ret, char *fmt, ...) __attribute__ ((__format__ (__printf__, 2, 0)));
extern int	perr_return(int ret, char *fmt, ...) __attribute__ ((__format__ (__printf__, 2, 0)));
extern void	moan(char *fmt, ...) __attribute__ ((__format__ (__printf__, 1, 0)));
extern void	warn(char *fmt, ...) __attribute__ ((__format__ (__printf__, 1, 0)));
extern void	info(char *fmt, ...) __attribute__ ((__format__ (__printf__, 1, 0)));
extern void	DBUG(char *fmt, ...) __attribute__ ((__format__ (__printf__, 1, 0)));
extern void	DBUGL(int lvl, char *fmt, ...) __attribute__ ((__format__ (__printf__, 2, 0)));
extern int	getstr(char *result, char *fmt, ...) __attribute__ ((__format__ (__printf__, 2, 0)));
extern int	get_value(int *value, int def, int oklo, int okhi, char *fmt, ...) __attribute__ ((__format__ (__printf__, 5, 0)));
/* extern void	strucpy(char *d, char *s); */
extern void	uppercase(char *s);
extern int	lookup_token(char *token, ...);

/* vlda.c */
extern int	GetRec_vlda(InRecord *record);
extern int	PutSym_vlda(FILE *file, uchar *data, int recsize);
extern int	PutRec_vlda(FILE *file, uchar *data, int recsize, ulong recstart);
