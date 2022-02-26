#ifndef LIBXMP_EFFECTS_H
#define LIBXMP_EFFECTS_H

/* Protracker effects */
#define FX_ARPEGGIO	0x00
#define FX_PORTA_UP	0x01
#define FX_PORTA_DN	0x02
#define FX_TONEPORTA	0x03
#define FX_VIBRATO	0x04
#define FX_TONE_VSLIDE  0x05
#define FX_VIBRA_VSLIDE	0x06
#define FX_TREMOLO	0x07
#define FX_OFFSET	0x09
#define FX_VOLSLIDE	0x0a
#define FX_JUMP		0x0b
#define FX_VOLSET	0x0c
#define FX_BREAK	0x0d
#define FX_EXTENDED	0x0e
#define FX_SPEED	0x0f

/* Fast tracker effects */
#define FX_SETPAN	0x08

/* Fast Tracker II effects */
#define FX_GLOBALVOL	0x10
#define FX_GVOL_SLIDE	0x11
#define FX_KEYOFF	0x14
#define FX_ENVPOS	0x15
#define FX_PANSLIDE	0x19
#define FX_MULTI_RETRIG	0x1b
#define FX_TREMOR	0x1d
#define FX_XF_PORTA	0x21

/* Protracker extended effects */
#define EX_FILTER	0x00
#define EX_F_PORTA_UP	0x01
#define EX_F_PORTA_DN	0x02
#define EX_GLISS	0x03
#define EX_VIBRATO_WF	0x04
#define EX_FINETUNE	0x05
#define EX_PATTERN_LOOP	0x06
#define EX_TREMOLO_WF	0x07
#define EX_SETPAN	0x08
#define EX_RETRIG	0x09
#define EX_F_VSLIDE_UP	0x0a
#define EX_F_VSLIDE_DN	0x0b
#define EX_CUT		0x0c
#define EX_DELAY	0x0d
#define EX_PATT_DELAY	0x0e
#define EX_INVLOOP	0x0f


#ifndef LIBXMP_CORE_DISABLE_IT
/* IT effects */
#define FX_TRK_VOL      0x80
#define FX_TRK_VSLIDE   0x81
#define FX_TRK_FVSLIDE  0x82
#define FX_IT_INSTFUNC	0x83
#define FX_FLT_CUTOFF	0x84
#define FX_FLT_RESN	0x85
#define FX_IT_BPM	0x87
#define FX_IT_ROWDELAY	0x88
#define FX_IT_PANSLIDE	0x89
#define FX_PANBRELLO	0x8a
#define FX_PANBRELLO_WF	0x8b
#define FX_HIOFFSET	0x8c
#define FX_IT_BREAK	0x8e	/* like FX_BREAK with hex parameter */
#endif

#define FX_SURROUND	0x8d	/* S3M/IT */
#define FX_REVERSE	0x8f	/* XM/IT/others: play forward/reverse */
#define FX_S3M_SPEED	0xa3	/* S3M */
#define FX_VOLSLIDE_2	0xa4
#define FX_FINETUNE	0xa6
#define FX_S3M_BPM	0xab	/* S3M */
#define FX_FINE_VIBRATO	0xac	/* S3M/PTM/IMF/LIQ */
#define FX_F_VSLIDE_UP	0xad	/* MMD */
#define FX_F_VSLIDE_DN	0xae	/* MMD */
#define FX_F_PORTA_UP	0xaf	/* MMD */
#define FX_F_PORTA_DN	0xb0	/* MMD */
#define FX_PATT_DELAY	0xb3	/* MMD */
#define FX_S3M_ARPEGGIO	0xb4
#define FX_PANSL_NOMEM	0xb5	/* XM volume column */

#define FX_VSLIDE_UP_2	0xc0	/* IT volume column volume slide */
#define FX_VSLIDE_DN_2	0xc1
#define FX_F_VSLIDE_UP_2 0xc2
#define FX_F_VSLIDE_DN_2 0xc3

#endif /* LIBXMP_EFFECTS_H */
