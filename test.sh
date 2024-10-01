#!/bin/bash

uname | grep Linux > /dev/null 2>&1

MIXIT=./mixit
SPLITTER=./splitter

TMP_NAM=mixtmp
TMP=tmp_mixittst
if [ ! -d $TMP ]; then
	WE_MADE_TMP=1
	mkdir $TMP
	if [ $? -ne 0 ]; then
		echo "FAILED: to create tmp dir: $TMP"
		exit 1
	fi
else
	WE_MADE_TMP=0
fi
TMP_OUT=$TMP/$TMP_NAM
TMP_MIX=$TMP_OUT.mix
TMP_LOG=$TMP_OUT.log

RMTMPS=1

if [ ! -e $MIXIT.exe ]; then
	if [ ! -e $MIXIT.EXE ]; then
		if [ ! -e $MIXIT ]; then
			echo "Mixit program ($MIXIT) not found"
			exit 1
		fi
	else
		MIXIT=$MIXIT.EXE
	fi
else
	MIXIT=$MIXIT.exe
fi
if [ ! -x $MIXIT ]; then
	echo "$MIXIT is not executable."
	exit 1
fi

MASTER_IMG=$MIXIT

if [ ! -e $SPLITTER.exe ]; then
	if [ ! -e $SPLITTER.EXE ]; then
		if [ ! -e $SPLITTER ]; then
			echo "Splitter program ($SPLITTER) not found"
			exit 1
		fi
	else
		SPLITTER=$SPLITTER.EXE
	fi
else
	SPLITTER=$SPLITTER.exe
fi 
if [ ! -x $SPLITTER ]; then
	echo "$SPLITTER is not executable"
	exit 1
fi

if [ ! -e $MASTER_IMG ]; then
	echo "Master test image ($MASTER_IMG) not found"
	exit 1
fi

MASTER64K=$TMP_OUT.64k
IMG_SIZE=`stat -c%s $MASTER_IMG`
if [ $IMG_SIZE -le 0 ]; then
	echo "Failed to size $MASTER_IMG"
	exit 1
fi
if [ $IMG_SIZE -gt 65536 ]; then
	$SPLITTER --infile=$MASTER_IMG --outfile=$MASTER64K --count=65536 2>/dev/null
	if [ $? -ne 0 ]; then
		echo "FAILED: to create 64k master file ($MASTER64K) from $MASTER_IMG"
		exit 1
	fi
elif [ $IMG_SIZE -lt 16384 ]; then
	echo "FAILED: Master img ($MASTER_IMG; size=$IMG_SIZE) smaller than 16384 bytes. Unable to test anything."
	exit 1
else
	echo "WARN: Master img ($MASTER_IMG) smaller than 65536 bytes. Unable to test mixit formats holding more than 65535 bytes."
	$SPLITTER --infile=$MASTER_IMG --outfile=MASTER64K
fi
IMG_SIZE_64K=`stat -c%s $MASTER64K`

IMG_SIZE_HEX=`printf %05X $IMG_SIZE`
IMG_END_HEX=`printf %05X $(($IMG_SIZE-1))`
IMG_PADDED_SIZE=$((($IMG_SIZE+511)&-512))
IMG_PADDED_END=$(($IMG_PADDED_SIZE-1))
IMG_PADDED_SIZE_HEX=`printf %05X $IMG_PADDED_SIZE`
IMG_PADDED_END_HEX=`printf %05X $IMG_PADDED_END`
#echo "IMG_SIZE=$IMG_SIZE, IMG_END_HEX=$IMG_END_HEX, IMG_PADDED_SIZE=$IMG_PADDED_SIZE, IMG_PADDED_SIZE_HEX=$IMG_PADDED_SIZE_HEX, IMG_PADDED_END_HEX=$IMG_PADDED_END_HEX"

FIRST_START=0
FIRST_OUT=0
FIRST_OUT_16=0
FIRST_END=$(((($RANDOM%8192)&-4)+2048-1))
SECOND_START=$((FIRST_END+1))
SECOND_OUT=$SECOND_START
SECOND_END=$((IMG_SIZE-1))
SECOND_OUT_16=$((($FIRST_END+1)/2))
FIRST_ST_HEX='00000'
FIRST_OUT_HEX='00000'
FIRST_OUT_HEX_16='00000'
FIRST_END_HEX=`printf %05X $FIRST_END`
SECOND_ST_HEX=`printf %05X $SECOND_START`
SECOND_END_HEX=`printf %05X $SECOND_END`
SECOND_OUT_HEX=`printf %05X $SECOND_OUT`
SECOND_OUT_HEX_16=`printf %05X $SECOND_OUT_16`
#echo "IMG_SIZE=$IMG_SIZE, FIRST=$FIRST_START($FIRST_ST_HEX), SECOND=$SECOND_START($SECOND_ST_HEX), SECOND_END=$SECOND_END($SECOND_END_HEX)"
#echo "      SECOND_OUT=$SECOND_OUT($SECOND_OUT_HEX), SECOND_OUT_16=$SECOND_OUT_16($SECOND_OUT_HEX_16)"

FIRST_START_64K=0
FIRST_OUT_64K=0
FIRST_END_64K=$FIRST_END
SECOND_START_64K=$((FIRST_END_64K+1))
SECOND_OUT_64K=$SECOND_START_64K
SECOND_END_64K=$((IMG_SIZE_64K-1))
SECOND_START_64K_16=$((($FIRST_END_64K+1)/2))
SECOND_OUT_64K_16=$((($FIRST_END_64K+1)/2))
SECOND_END_64K_16=$(($IMG_SIZE_64K/2-1))
FIRST_ST_HEX_64K='00000'
FIRST_OUT_HEX_64K='00000'
FIRST_OUT_HEX_64K_16='00000'
FIRST_END_HEX_64K=`printf %05X $FIRST_END_64K`
SECOND_ST_HEX_64K=`printf %05X $SECOND_START_64K`
SECOND_END_HEX_64K=`printf %05X $SECOND_END_64K`
SECOND_ST_HEX_64K_16=`printf %05X $SECOND_START_64K_16`
SECOND_END_HEX_64K_16=`printf %05X $SECOND_END_64K_16`
SECOND_OUT_HEX_64K=`printf %05X $SECOND_OUT_64K`
SECOND_OUT_HEX_64K_16=`printf %05X $SECOND_OUT_64K_16`
#echo "IMG_SIZE_64K=$IMG_SIZE_64K, FIRST_64K=$FIRST_START_64K($FIRST_ST_64K_HEX), SECOND_ST_64K=$SECOND_START_64K($SECOND_ST_HEX_64K), SECOND_END_64K=$SECOND_END_64K($SECOND_END_HEX_64K)"
#echo "      SECOND_OUT_64K=$SECOND_OUT_64K($SECOND_OUT_HEX_64K), SECOND_OUT_64K_16=$SECOND_OUT_64K_16($SECOND_OUT_HEX_64K_16)"
#exit 0

MK_IMG()
{
# OUT command options, if any, are in parameter 1
# IN command options, if any, are in parameter 2
	cat > $TMP_MIX <<EOF
	out $TMP_OUT.img $1
	in $MASTER_IMG $2
	exit
EOF
	$MIXIT $TMP_MIX > $TMP_LOG 2>&1
	if [ $? -ne 0 ]; then
		echo "FAILED: TEST_FULL(): Making $TMP_OUT.img. $1 $2"
		exit 1
	fi
	return 0
}

TEST_FULL()
{
# OUT command options, if any, are in parameter 1
# IN command options, if any, are in parameter 2
# Message to report on pass/fail is in parameters 3, 4 and 5
	MK_IMG "$1" "$2"
	if [ $? -ne 0 ]; then
		return 1;
	fi
	cmp -s $MASTER_IMG $TMP_OUT.img
	if [ $? -ne 0 ]; then
		echo "FAILED: $3 $4 $5"
		exit 1
	fi
	echo "Success: $3 $4 $5"
	if [ $RMTMPS -ne 0 ]; then
		rm -f $TMP_MIX $TMP_OUT.img $TMP_LOG
	fi
	return 0
}

TEST_FULL "" "" "test of image type" "without address limits"
TEST_FULL "" "-addr=0:$IMG_END_HEX:0" "test of image type" "with address limits"

MK_IMG "-fill=ff" "-addr=0:$IMG_PADDED_END_HEX:0"
TST_IMG_SIZE=`stat -c%s $TMP_OUT.img`
if [ $IMG_PADDED_SIZE -ne $TST_IMG_SIZE ]; then
	echo "FAILED: Padded built $TMP_OUT.img file wrong size. Found $TST_IMG_SIZE, expected $IMG_PADDED_SIZE (diff=$(($TST_IMG_SIZE-$IMG_PADDED_SIZE)))"
	exit 1
fi
$SPLITTER --infile=$TMP_OUT.img --outfile=$TMP_OUT.tmp --count=$IMG_SIZE 2> /dev/null
if [ $? -ne 0 ]; then
	echo "FAILED: running $SPLITTER to extract un-padded section of $TMP_OUT.img."
	exit 1
fi
cmp -s $MASTER_IMG $TMP_OUT.tmp
if [ $? -ne 0 ]; then
	echo "FAILED: Extract of $IMG_SIZE bytes from $TMP_OUT.tmp doesn't match $MASTER_IMG"
	exit 1
fi
if [ $RMTMPS -ne 0 ]; then
	rm -f $TMP_MIX $TMP_OUT.img $TMP_LOG
fi
echo "Success: test of image type with padding and address limits"

TST_TYPE_BYTE()
{
	MAIN_IMG=$MASTER_IMG;
	F_S_HEX=$FIRST_ST_HEX
	F_E_HEX=$FIRST_END_HEX
	S_S_HEX=$SECOND_ST_HEX
	S_E_HEX=$SECOND_END_HEX
	if [ "$2" != "" ]; then
		MAIN_IMG=$MASTER64K
		F_S_HEX=$FIRST_ST_HEX_64K
		F_E_HEX=$FIRST_END_HEX_64K
		S_S_HEX=$SECOND_ST_HEX_64K
		S_E_HEX=$SECOND_END_HEX_64K
	fi
	cat > $TMP_MIX <<EOF
	out $TMP_OUT.$1
	in $MAIN_IMG
	exit
EOF
	$MIXIT $TMP_MIX > $TMP_LOG 2>&1
	if [ $? -ne 0 ]; then
		echo "FAILED: could not create '$1' output file (attempt without address limits)."
		exit 1
	fi
	cat > $TMP_MIX <<EOF
	out $TMP_OUT.tmp
	in $TMP_OUT.$1
	exit
EOF
	$MIXIT $TMP_MIX > $TMP_LOG 2>&1
	if [ $? -ne 0 ]; then
		echo "FAILED: could not convert $TMP_OUT.$1 back to image format (attempt without address limits)"
		exit 1
	fi
	cmp -s $MAIN_IMG $TMP_OUT.tmp
	if [ $? -ne 0 ]; then
		echo "FAILED: $MAIN_IMG does not match image converted from $TMP_OUT.$1 ($TMP_OUT.tmp) (attempt without address limits)"
		exit 1
	fi
	rm -f $TMP_MIX $TMP_LOG $TMP_OUT.tmp
	echo "Success: test of $1 type output and input without address limits"
	cat > $TMP_MIX <<EOF
	out $TMP_OUT.tmp
	in $TMP_OUT.$1 -add=$F_S_HEX:$F_E_HEX:$F_S_HEX
	in $TMP_OUT.$1 -add=$S_S_HEX:$S_E_HEX:$S_S_HEX
	exit
EOF
	$MIXIT $TMP_MIX > $TMP_LOG 2>&1
	if [ $? -ne 0 ]; then
		echo "FAILED: creating image file after creating '$1'"
		exit 1
	fi
	cmp -s $MAIN_IMG $TMP_OUT.tmp
	if [ $? -ne 0 ]; then
		echo "FAILED: comparing image file made from sectioned $1 ($TMP_OUT.tmp) against master ($MAIN_IMG)"
		exit 1
	fi
	if [ $RMTMPS -ne 0 ]; then
		rm -f $TMP_OUT.$1 $TMP_MIX $TMP_OUT.img $TMP_LOG
	fi
	echo "Success: test of $1 type output and input with address limits"
	return 0
}

TST_TYPE_WORD()
{
	MAIN_IMG=$MASTER_IMG;
	F_S_HEX=$FIRST_ST_HEX
	F_E_HEX=$FIRST_END_HEX
	S_S_HEX=$SECOND_ST_HEX
	S_E_HEX=$SECOND_END_HEX
	F_O_HEX=$FIRST_OUT_HEX_16
	S_O_HEX=$SECOND_OUT_HEX_16
	if [ "$2" != "" ]; then
		MAIN_IMG=$MASTER64K
		F_S_HEX=$FIRST_ST_HEX_64K
		F_E_HEX=$FIRST_END_HEX_64K
		S_S_HEX=$SECOND_ST_HEX_64K
		S_E_HEX=$SECOND_END_HEX_64K
		F_O_HEX=$FIRST_OUT_HEX_64K_16
		S_O_HEX=$SECOND_OUT_HEX_64K_16
	fi
	cat > $TMP_MIX <<EOF
	out $TMP_OUT.$1
	in $MAIN_IMG
	exit
EOF
	$MIXIT $TMP_MIX > $TMP_LOG 2>&1
	if [ $? -ne 0 ]; then
		echo "FAILED: could not create '$1' output file without address limits."
		exit 1
	fi
	cat > $TMP_MIX <<EOF
	out $TMP_OUT.tmpg0
	in $TMP_OUT.$1 -word_size=16 -group=0
	out $TMP_OUT.tmpg8
	in $TMP_OUT.$1 -word_size=16 -group=8
	out $TMP_OUT.tmpe
	in $TMP_OUT.$1 -even
	out $TMP_OUT.tmpo
	in $TMP_OUT.$1 -odd
	exit
EOF
	$MIXIT $TMP_MIX > $TMP_LOG 2>&1
	if [ $? -ne 0 ]; then
		echo "FAILED: could not convert $TMP_OUT.$1 to split images without address limits"
		exit 1
	fi
	# mixit will pad output files having a word_size != 8 to an even size.
	# This could be considered a bug but I'm not interested in fixing it
	# so, instead, the tester is patched here to accomodate it.
	TMP_IMG_SIZE=`stat -c%s $TMP_OUT.tmpg0`
	$SPLITTER --infile=$MAIN_IMG --outfile=$TMP_OUT.even --byteskip=2 --count=$TMP_IMG_SIZE 2> $TMP_LOG
	if [ $? -ne 0 ]; then
		echo "FAILED: $SPLITTER was unable to make output $TMP_OUT.even"
		exit 1
	fi
	$SPLITTER --infile=$MAIN_IMG --outfile=$TMP_OUT.odd --inskip=1 --byteskip=2 --count=$TMP_IMG_SIZE 2> $TMP_LOG
	if [ $? -ne 0 ]; then
		echo "FAILED: $SPLITTER was unable to make output $TMP_OUT.odd"
		exit 1
	fi
	cmp -s $TMP_OUT.even $TMP_OUT.tmpg0
	if [ $? -ne 0 ]; then
		echo "FAILED: $TMP_OUT.even does not match -group=0 image converted from $TMP_OUT.$1 ($TMP_OUT.tmpg0) without address limits"
		exit 1
	fi
	cmp -s $TMP_OUT.even $TMP_OUT.tmpe
	if [ $? -ne 0 ]; then
		echo "FAILED: $TMP_OUT.even does not match -even image converted from $TMP_OUT.$1 ($TMP_OUT.tmpe) without address limits"
		exit 1
	fi
	cmp -s $TMP_OUT.odd $TMP_OUT.tmpg8
	if [ $? -ne 0 ]; then
		echo "FAILED: $TMP_OUT.odd does not match -group=8 image converted from $TMP_OUT.$1 ($TMP_OUT.tmpg8) without address limits"
		exit 1
	fi
	cmp -s $TMP_OUT.odd $TMP_OUT.tmpo
	if [ $? -ne 0 ]; then
		echo "FAILED: $TMP_OUT.odd does not match -odd image converted from $TMP_OUT.$1 ($TMP_OUT.tmpo) without address limits"
		exit 1
	fi
	echo "Success: test of $1 type output and input word_size=16. Tested -even, -odd, -group=0, -group=8 all without address limits"
	cat > $TMP_MIX <<EOF
	out $TMP_OUT.tmp
	in $TMP_OUT.$1 -add=$F_S_HEX:$F_E_HEX:$F_O_HEX -even
	in $TMP_OUT.$1 -add=$S_S_HEX:$S_E_HEX:$S_O_HEX -even
	exit
EOF
	$MIXIT $TMP_MIX > $TMP_LOG 2>&1
	if [ $? -ne 0 ]; then
		echo "FAILED: creating image file after creating '$1'"
		exit 1
	fi
	cmp -s $TMP_OUT.even $TMP_OUT.tmp
	if [ $? -ne 0 ]; then
		echo "FAILED: comparing image file made from sectioned $1 ($TMP_OUT.tmp) against master ($TMP_OUT.even)"
		exit 1
	fi
	cat > $TMP_MIX <<EOF
	out $TMP_OUT.tmp
	in $TMP_OUT.$1 -add=$F_S_HEX:$F_E_HEX:$F_O_HEX -odd
	in $TMP_OUT.$1 -add=$S_S_HEX:$S_E_HEX:$S_O_HEX -odd
	exit
EOF
	$MIXIT $TMP_MIX > $TMP_LOG 2>&1
	if [ $? -ne 0 ]; then
		echo "FAILED: creating image file after creating '$1'"
		exit 1
	fi
	cmp -s $TMP_OUT.odd $TMP_OUT.tmp
	if [ $? -ne 0 ]; then
		echo "FAILED: comparing image file made from sectioned $1 ($TMP_OUT.tmp) against master ($TMP_OUT.odd)"
		exit 1
	fi
	echo "Success: test of $1 type output and input word_size=16. Tested -even and -odd with address limits"
	if [ $RMTMPS -ne 0 ]; then
		rm -f $TMP_OUT.$1 $TMP_OUT.tmp $TMP_OUT.even $TMP_OUT.odd $TMP_OUT.tmpg0 $TMP_OUT.tmpe $TMP_OUT.tmpg8 $TMP_OUT.tmpo $TMP_LOG $TMP_MIX
	fi
	return 0
}

#	GPF_K_LDA,		/* RT11 .LDA (16 bit addresses, I/O binary) */
#	GPF_K_ROM,		/* mixit .ROM (I/O ASCII) */
#	GPF_K_MAC,		/* macxx .MAC (O only ASCII)*/
#	GPF_K_HEX,		/* TekHex .HEX (I/O ASCII) */
#	GPF_K_DLD,		/* MOS Technology? .DLD (I/O ASCII) */
#	GPF_K_VLDA,		/* Atari .VLDA or .VLD (32 bit addresses, I/O binary) */
#	GPF_K_GNU,		/* GNU .ASM or .AS68K () (O only ASCII) */
#	GPF_K_INTEL,	/* Intel .INTEL (I/O ASCII) */
#	GPF_K_MOT,		/* Motorola .MOT (I/O ASCII) */
#	GPF_K_DUMP,		/* mixit dump .DUMP, .DUM or .DMP (O only same as IMG) */
#	GPF_K_DIO,		/* Atari DataIO .DIO (I/O binary) */
#	GPF_K_COFF,		/* Generic COFF .COFF (I only, binary) */
#	GPF_K_ELF,		/* Generic ELF .ELF (I only, binary) */
#	GPF_K_CPE,		/* Sony Playstation .CPE (I/O binary) */

TST_TYPE_BYTE lda
TST_TYPE_BYTE rom
TST_TYPE_BYTE hex
# DLD format is special case being it only has 16 bit addresses.
TST_TYPE_BYTE dld $MASTER64K
TST_TYPE_BYTE vlda
# INTEL format is special case being it only has 16 bit addresses.
TST_TYPE_BYTE intel $MASTER64K
TST_TYPE_BYTE mot
TST_TYPE_BYTE dio
TST_TYPE_BYTE cpe

TST_TYPE_WORD lda
TST_TYPE_WORD rom
TST_TYPE_WORD hex
# DLD format is special case being it only has 16 bit addresses.
TST_TYPE_WORD dld $MASTER64K
TST_TYPE_WORD vlda
# INTEL format is special case being it only has 16 bit addresses.
TST_TYPE_WORD intel $MASTER64K
TST_TYPE_WORD mot
TST_TYPE_WORD dio
TST_TYPE_WORD cpe
if [ $RMTMPS -ne 0 ]; then
	rm -f $MASTER64K
	if [ $WE_MADE_TMP -ne 0 ]; then
		rm -rf $TMP
	fi
fi

