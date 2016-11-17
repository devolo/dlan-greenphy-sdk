/*
 * Copyright (c) 2012, devolo AG, Aachen, Germany.
 * All rights reserved.
 *
 * This Software is part of the devolo GreenPHY-SDK.
 *
 * Usage in source form and redistribution in binary form, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Usage in source form is subject to a current end user license agreement
 *    with the devolo AG.
 * 2. Neither the name of the devolo AG nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 3. Redistribution in binary form is limited to the usage on the GreenPHY
 *    module of the devolo AG.
 * 4. Redistribution in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * debug.c
 *
 */

#ifndef DEBUG
#define DEBUG
#endif

#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "uart.h"
#include "debug.h"
#include "debugConfig.h"

#define false 0
#define true 1

#define NO_MULTI_DELIM 'z'     /* 'z' as delimiter means no delimiter */

#define putc(c,fp)  (fp)->Ops->OutFunc(fp,c)
#define fgetc(fp)   (fp)->Ops->InFunc(fp)
#define fstatc(fp)  (fp)->Ops->InStat(fp)

#ifndef NULL
  #ifdef __cplusplus
    #define NULL          0L            /* Null Zeiger                    */
  #else
    #define NULL          ((void *)0)   /* Null Zeiger                    */
  #endif
#endif


#define mNetswaps(a) ( ((a)>>8) | (((a)&0xff) << 8) )
#define mNetswapl(b) ( (((b)&0xff000000) >> 24) \
                     | (((b)&0x00ff0000) >> 8) \
                     | (((b)&0x0000ff00) << 8) \
                     | (((b)&0x000000ff) << 24))


/*
 * ntohs htons ntohl, htonl macros yielding compile-time constants
 * from constants
 */

#include <byteorder.h>

#ifdef BYTE_ORDER_BIG_ENDIAN
#endif
#ifdef BYTE_ORDER_LITTLE_ENDIAN
#define ENDIANESS_LITTLE
#endif

#ifdef ENDIANESS_LITTLE

  #define mNet2Host16(a) mNetswaps(a)
  #define mNet2Host32(a) mNetswapl(a)
  #define mHost2Net16(a) mNetswaps(a)
  #define mHost2Net32(a) mNetswapl(a)

#else

  #ifndef ENDIANESS_BIG
    #warning (frame.h)  unknown ENDIANESS - big endianness (net byte order) assumed
  #endif

  #define mNet2Host16(a) (a)
  #define mNet2Host32(a) (a)
  #define mHost2Net16(a) (a)
  #define mHost2Net32(a) (a)

#endif

int isdigit2(int c) {return c<'0'? 0 : (c>'9' ? 0 : c);}

/*
 ************************************************************************
 * a forward reference to types still to be defined
 ************************************************************************
 */
struct IosFile;

/*
 ************************************************************************
 *
 * types of functions used in files (as casting is required)
 *
 ************************************************************************
 */

  /* a file output function writes one character to the file
   * the write may return one of the following values :
   * 0 - the write succeeded OK
   * WOULDBLOCK - file is nonblocking and the buffer is full
   * EOF          file is closed
   */
typedef int16_t (* tpIosFileOutFunc)(struct IosFile *, uint8_t);

   /* a file input function returns one of the following values
    * WOULDBLOCK  file is nonblocking and buffer is empty
    * EOF         file is closed
    * 0x00-0xff   received character
    */

typedef int16_t (* tpIosFileInFunc)(struct IosFile *);

   /* file status functions are used both for input and
    * output status of a file :
    * used as input status function it returns either the
    * number of characters ready for collection without
    * the risk to block on the file or EOF if the file is closed
    *
    * used as status function for output it returns the
    * actually remaining space of the file, i.e. the number
    * of characters the job could write into the file without the
    * risk of blocking on the file. If the file is closed this call
    * also returns EOF
    */

typedef int32_t (* tpIosFileStatFunc)(struct IosFile *);

   /* the following prototype is used for IoCtrl Functions
    * this mechanism is still a bit fresh, so expect modifications
    * in the nearer future
    */

typedef uint32_t (* tpIosFileCtrlFunc)(struct IosFile *,
                                   uint16_t Command,
                                   uint32_t Param);

struct sBQueBlk;

typedef void (* tpIosFileOutBlkFunc)(struct IosFile *,struct sBQueBlk*);

/*
 ************************************************************************
 * the file operators structure ( a structure of all functions
 * required to operate a file)
 ************************************************************************
 */

typedef struct IosFileOps
{
    tpIosFileOutFunc   OutFunc;
    tpIosFileStatFunc  OutStat;
    tpIosFileInFunc    InFunc;
    tpIosFileStatFunc  InStat;
    tpIosFileCtrlFunc  IoCtrl;
} tIosFileOps;

#define mIosFilDcl(Name, Ops, pTxData, pRxData, pCtrlData)  \
        tIosFile Name = { Ops, pTxData, pRxData, pCtrlData }

static tIosFileOps IosStringFileOps;


#define STRING_FILE(name, pc) mIosFilDcl(name, &IosStringFileOps , (void *)pc, (void *)pc, NULL)


/*
 ************************************************************************
 * the file structure itself
 ************************************************************************
 */

typedef struct IosFile
{
   tIosFileOps       * Ops;        /* pointer to file ops table */
   void              * pTxData;    /* pointer to assoc. transmit structure */
   void              * pRxData;    /* pointer to assoc. receive data structure */
   void              * pCtrlData;  /* pointer to file-specific ctrl struct */
} tIosFile;


/* ---- fprintf Typen ----------------------------------------------------- */

#define VAL_MAX  34

#define DEACTIV   0
#define ACTIV     1

/* ---- offset fuer Upper- Lower-case in  FormChr[] ----------------------- */

#define UPPER_C 0
#define LOWER_C 1

/* ---- lokale Typen ------------------------------------------------------ */

typedef enum
{
  eSTART=0,
  eFIRST,
  eFORM
} tCState;

typedef enum
{
  e8BIT  = 0,
  e16BIT = 1,
  e32BIT = 2
} tDataLen;


typedef union
{
  long      l;
  uint16_t  U16;      /* Notwendige Members fuer das Big-Endian Modell, /DHagel */
  uint8_t   U8;
  void*     p;
  char*     pC;
  uint8_t*  pU8;
} tArg;


typedef struct FmtVars
{
 const char*  pForm;                         /* Zeiger auf FormChr           */
       uint8_t*  pValPos;                       /*^ auf FormatierPuffer         */
       uint16_t  Min;
       uint16_t  Max;
       int8_t    Cnt;
       int8_t    DateLen;
       int       Left;
       int       Zero;
       int       Alt;
       uint8_t   HexPref;
       uint8_t   Sign;
       uint8_t   ValString[VAL_MAX+1];          /* FormatierPuffer              */
} tFmtVars;

static const char* FormChr[]=
{
  "0123456789ABCDEF",
  "0123456789abcdef"
};
/*
 ************************************************************************
 * put a char to a string represented by a file
 ************************************************************************
 */

static int16_t IosStringPutchar (tIosFile *fp, uint8_t c)
{
   uint8_t * pD = (uint8_t*) fp->pTxData;
   *pD++=c;
   *pD=0;      /* always write the trailing 0 */

   fp->pTxData = (void *) pD;

   return 0;
}


/*
 ************************************************************************
 * return whether file ready for write (always true for strings)
 ************************************************************************
 */
static uint32_t IosStringIoCtl(tIosFile *pFile, uint16_t Code, uint32_t Value)
{
   (void)pFile;(void)Code;(void)Value;
   return 0;
}
/*
 ************************************************************************
 * return whether file ready for write (always true for strings)
 ************************************************************************
 */

static int32_t IosStringOutStat (tIosFile * fp)
{
   (void)fp;
   return 1;    /* always ready for output */
}

/*
 ************************************************************************
 * get next char from string
 ************************************************************************
 */

static int16_t IosStringGetchar (tIosFile *fp)
{
   uint8_t * pD = (uint8_t*) fp->pRxData;
   uint8_t    D = * pD;
   if (D != 0)
   {
      ++pD;
      fp->pRxData = (void *) pD;
   }
   return D;
}

/*
 ************************************************************************
 * return true if more chars in string
 ************************************************************************
 */

static int32_t IosStringInStat (tIosFile * fp)
{
   uint8_t NextChar = *(uint8_t*)(fp->pRxData);
   return (NextChar==0) ? 0 : 1;
}


/*
 ************************************************************************
 * File ops table for strings
 ************************************************************************
 */

static tIosFileOps IosStringFileOps = { IosStringPutchar,
                                 IosStringOutStat,
                                 IosStringGetchar,
                                 IosStringInStat,
                                 IosStringIoCtl
                                 };
/* ------------------------------------------------------------------------
 * $Funktion:   PrimForm2
 * $Aufgabe.:   Gibt eine Zahl aus; entweder als Dezimal- oder als Hex-Zahl.
 * $Resultat:   %
 * $Hinweis.:   -
 * $Beispiel:   -
 * ------------------------------------------------------------------------ */

static void PrimForm2
( tFmtVars* p,
  uint32_t      Zahl,
  uint8_t       Base )
{
  uint8_t* pChr;

  pChr = p->ValString+VAL_MAX+1;
  *--pChr='\0';                             /* string terminator            */

  if( Base == 16 )
  {
    uint16_t i;

    for( i = 0; ; i++ )
    {
      if( p->Max && ( i >= p->Max ))
        break;

      *--pChr= p->pForm[Zahl&0x0f];
      Zahl>>= 4;
      if( !Zahl && !p->Zero )
        break;

    }
  }
  else
  {
    for(;;)
    {
      *--pChr= p->pForm[Zahl%Base];
      Zahl/=Base;
      if( !Zahl) break;
    }
  }
  p->Cnt=(int16_t)( p->ValString - pChr +VAL_MAX); /* benoetigte Ziffern        */
  p->pValPos = pChr;                           /* Zeiger zur ausgabe        */
} /* PrimForm2 */


/* ------------------------------------------------------------------------
 * $Funktion:   ValOut
 * $Aufgabe.:   Formattierte Ausgabe
 * $Resultat:   int                         ; ???
 * $Hinweis.:   -
 * $Beispiel:   -
 * ------------------------------------------------------------------------ */

static int ValOut
( tFmtVars* p,
  tIosFile* fp )
{ int   FieldSz;
  int   FieldSz_2;
  int   n;
  int   ChrCnt;
  unsigned char* pChr;

  ChrCnt = 0;
  pChr = p->pValPos;

  FieldSz   = p->Cnt;
  FieldSz_2 =
  FieldSz   = (FieldSz>p->Min) ? FieldSz:p->Min;    /* Max()                     */

  if(p->Sign)
    FieldSz_2+=1;
  if (p->HexPref)
    FieldSz_2+=2;

  if(
     !(p->Left||p->Zero) &&                     /* rechts ausgerichtet       */
       p->Max                                   /* Feldbreite angegeben      */
    )
    for(n=p->Max-FieldSz_2; n>0 ; n--)
    {
      putc(' ',fp);
      ChrCnt++;
    }

  if(p->Sign)
  {
    putc(p->Sign,fp);
    ChrCnt++;
  }

  if (p->HexPref)
  {
    putc ('0',fp);
    putc (p->HexPref,fp);
    ChrCnt+=2;
  }

  if(!p->Left && p->Zero)                       /* Right adjusting Zero-     */
    for(n=p->Max-FieldSz ; n>0 ; n--)           /* Padding                   */
    {
      putc('0',fp);                            /* fuellen                   */
      ChrCnt++;
    }
                                                /* leading Zeros             */
  for(n=p->Min - p->Cnt; n>0 ; n--)             /* Padding                   */
  {
    putc('0',fp);                              /* fuellen                   */
    ChrCnt++;
  }

  n=  p->Cnt;
  ChrCnt+=n;
  while (n--)
  {
    putc(*pChr++,fp);                          /* ASCII Zeichen ausgeben    */
  } /* while */


  if(p->Left)                                   /*left adjusting             */
    for( n=p->Max-FieldSz_2; n>0 ; n--)         /* Padding                   */
    {
      putc(' ',fp);                            /* blank                     */
      ChrCnt++;
    }
  return ChrCnt;                             /* verarbeitete Zeichen zurueck */
}


/**************************************************************
 *                                                            *
 *  *   *           *     ***   *   *                  *      *
 *  **  *           *    *   *  *   *                  *      *
 *  * * *   ***    ***       *  *   *   ***    ***    ***     *
 *  *  **  *   *    *       *   *****  *   *  *        *      *
 *  *   *  *****    *      *    *   *  *   *   ***     *      *
 *  *   *  *        *     *     *   *  *   *      *    *      *
 *  *   *   ***     *    *****  *   *   ***    ***     *      *
 *                                                            *
 *                                                            *
 **************************************************************/

static inline uint8_t xBindFrmPN2H8 (void* netdata)
{
  return ((uint8_t*)netdata)[0];
}

static inline uint16_t xBindFrmPN2H16 (void* netdata)
{
  return (((uint8_t*)netdata)[0] << 8)
       |  ((uint8_t*)netdata)[1];
}

static inline uint32_t xBindFrmPN2H32 (void* netdata)
{
  return (((uint8_t*)netdata)[0] << 24)
       | (((uint8_t*)netdata)[1] << 16)
       | (((uint8_t*)netdata)[2] <<  8)
       |  ((uint8_t*)netdata)[3];
}


/* ------------------------------------------------------------------------
 * $Funktion:   PrimMulti
 * $Aufgabe.:   Multiple Hex/Decimal/Binary Ausgabe
 * $Resultat:   int                        ; Anzahl auszugebender Zeichen
 * $Hinweis.:
 * $Beispiel:   -
 * ------------------------------------------------------------------------ */

static int PrimMulti (tFmtVars * p,
               tIosFile * fp,
               uint8_t      * src,
               uint8_t        base,
               uint8_t        delimiter)
{
   int Cnt     = 0;
   int nrepeat = p->Max;
   uint32_t        Current;

   p->Max      = p->Min;

   while (nrepeat--)
   {
      switch (p->DateLen)
      {
         case e8BIT :
            Current = xBindFrmPN2H8(src);
            ++src;
            break;
         case e16BIT :
            Current = xBindFrmPN2H16(src);
            src +=2;
            break;
         default :
            Current = xBindFrmPN2H32(src);
            src +=4;
            break;
      }
      PrimForm2(p,Current,base);
      Cnt+=ValOut (p,fp);

      if ((nrepeat > 0) && (delimiter != NO_MULTI_DELIM))
      {
         putc(delimiter,fp);
         ++Cnt;
      }
   }
   return Cnt;
}


/* ------------------------------------------------------------------------ */


/* ------------------------------------------------------------------------
 * $Funktion:   xIosPrnKern
 * $Aufgabe.:   Kernroutine fuer die Formatierte Ausgabe
 *              Wird in sprintf, fprintf, xIosPrnPrin, xMonPrnPrint benutzt
 *
 *              Es wird folgender Formatstring unterstuetzt:
 *              %[Flags][width][.precision][{h|l}]type
 *
 *              Als Flags werden akzeptiert:
 *               '-'     Ausrichtung nach links
 *               '+'     immer mit vorzeichen
 *               ' '     (blank) wie Vorzeichen jedoch mit ' ' vorbelegt
 *                       Beim Blank wird bei positiven Ziffer mindestens
 *                       ein ' ' als Vorzeichen ausgegeben. Wichtig wenn
 *                       Positive Zahlen auch ohne Vorzeichen das gleiche
 *                       Format erhalten sollen wie die Negativen.
 *               '0'     Alle nichtbenutzten Felder von [width] mit '0'
 *                       fuellen. Wird bei '-'ignoriert.
 *               '#'     Alternative Darstellung
 *                        Prefix '0' bei [O,o]
 *                        Prefix '0x bei [x]
 *                        Prefix '0X bei [X]
 *               [width] Feldbreite, Anzahl der Zeichen die mindestens
 *                       ausgegeben werden. Ein '*' als with sorgt dafuer,
 *                       das die Feldbreite aus der Parameterliste geholt
 *                       wird.
 *               [.precision]
 *                       Anzahl der Zeichen die mindestens fuer die Dar-
 *                       Stellung der Zahl mindestens benutzt werden.
 *                       Ein '*' als precision sorgt dafuer, das die Feld-
 *                       Breite aus der Parameterliste geholt wird.
 *                       Bei dem type %s bestimmt die precision die Anzahl
 *                       der Zeichen die maximal aus dem Quellstr ins Ziel
 *                       kopiert werden.
 *               [{h|l}] Laengenprefix beeinflusst die Datendatenlaenge des
 *                       Parameters der vom Stack geholt wird.
 *                       !!!Wichtig!!!
 *                       Bei Prefix 'h' wird auch ein int vom Stack geholt
 *                       da der ARM-Compiler allen Datentypen die kleiner
 *                       sind als int, als int auf den Stack legt.
 *
 *               type    d,i    long/int signed   dezimal
 *                       u      long/int unsigned dezimal
 *                       O      long     unsigned Oktal
 *                       o      long/int unsigned Oktal
 *                       x,p    long/int unsigned hex.    verwende abcdef
 *                       X      long/int unsigned hex.    verwende ABCDEF
 *                       c      char
 *                       s      ^ auf  o-terminierten String
 *                       n      ^ auf Integer
 *                       gibt die Anzahl der ausgegebenen Zeichen zurueck
 *
 *               f,e,
 *               E,g,G   sind nicht implementiert
 *
 *              Zusaetzlich zum Standard
 *               m       Multichar  ausgeben
 *               q       Quadchar   ausgeben
 * $Resultat:   int                         ; # Bisher ausgegebener Zeichen
 * $Hinweis.:   -
 * $Beispiel:   -
 * ------------------------------------------------------------------------ */

uint32_t xIosPrnKern
(       tIosFile* fp,
        va_list   Ap,
  const char*     fmt )
{ tFmtVars Par;
  uint32_t     Cnt = 0;                         /* Zeichenzaehler               */
  tCState  State = eSTART;
  tArg     Arg;

  for(;*fmt;fmt++)
  {
    if (*fmt!='%' && State==eSTART)
    {
      putc(*fmt,fp);                              /* Ausgabe              */
      Cnt++;                                       /* zeichenzaehler       */
    }
    else                                           /*  formatierung        */
    {
      switch(State)
      {
        case eSTART:
          //xIosPrnStr("\r\nin START");
          ++fmt;                               /* ptr auf Format Chr       */
          Par.Min=0;
          Par.Max=0;
          Par.DateLen=e32BIT;
          Par.Zero = false;
          Par.Left = false;
          Par.Alt  = false;
          Par.HexPref = '\0';
          Par.Sign ='\0';
          State= eFIRST;
          /*no break*/

        case eFIRST:
          //xIosPrnStr("\r\nin FIRST  *fmt=");
          //xIosPrnChr(*fmt);
          switch(*fmt)                         /* oder Formatzeichen        */
          {
            case '.':                          /* Precision                 */
              {
                int n;
                if (*++fmt=='*')               /* Prec als Parameter        */
                {
                  n=va_arg(Ap,int);
                }
                else
                {
                  n=0;
                  while(isdigit2(*fmt))
                  {
                    n = n * 10+(*fmt-'0');
                    fmt++;
                  }
                  --fmt;                       /* Zeichen nochmal bearbeiten */
                }
                Par.Min=n;                     /* Prec uebernehmen           */
              }
              State=eFORM;
              continue;

            case '#':                           /* alternative darstellung    */
              Par.Alt=true;
              continue;

            case '%':
              --fmt;                           /* Nochmal bearbeiten         */
              State=eFORM;                     /* wichtig : keinen Parameter */
              continue;                        /* lesen                      */

            case ' ':                          /* vorzeichen nur wenn noch   */
              if (Par.Sign)                    /* nicht belegt               */
                Par.Sign =' ';
              continue;                        /* for */

            case '+':                          /* Immer mit vorzeichen       */
              Par.Sign ='+';
              continue;                        /* for */

            case '-':                          /* ausrichtung nach links     */
              Par.Left = true;                 /* ist activ                  */
              continue;

            case '0':                         /* 0 ist als flag zu behandeln */
              Par.Zero = true;                /* ist activ                   */
              continue;

            case '*':
              {
                int n=va_arg(Ap,int);          /* Width von Parameterliste  */
                Par.Max=(n<0) ? -n : n;        /* betrag von Width          */
              }
              continue;

            default:
              if(isdigit2(*fmt))                /* Ist Ziffer                */
              {
                int n=0;
                while(isdigit2(*fmt))
                {
                  n = n * 10+(*fmt-'0');
                  fmt++;
                }
                Par.Max=n;                     /* maximale Feldbreite       */
              }
              else
              {                                     /* weiter in LEN              */
                //State=LEN;                   /*    bearbeiten             */
                State=eFORM;                   /*    bearbeiten             */
              }
              --fmt;                           /* zeichen nochmal bearbeiten*/
              continue;                        /* for                       */
          }                                    /* switch                    */
        /*no break*/
        case eFORM:
          Par.pForm=FormChr[LOWER_C];          /* Default setzen            */
          if(Par.Max>=VAL_MAX)
            Par.Max=VAL_MAX-1;
//          if(Par.Min>Par.Max)
//            Par.Max=Par.Min;
          //xIosPrnStr("\r\nin FORM");
          switch(*fmt)
          {
            case 'n':
              *va_arg(Ap,int*)=Cnt;            /* benutzt immer Ptr->int    */
              State=eSTART;
              continue;

            case 'l':                          /* long                      */
              Par.DateLen=e32BIT;
              continue;

            case 'b':                          /* byte                      */
              Par.DateLen=e8BIT;               /*bei ARM7 nicht genutzt     */
              continue;

            case 'h':                          /* half                      */
              Par.DateLen=e16BIT;              /* bei ARM7 nicht genutzt    */
              continue;
            case 's':
              Arg.pC=va_arg(Ap,char *);
              break;

            case 'd':                          /* signed   int              */
            case 'i':                          /* signed   int              */
            case 'u':                          /* unsigned int              */
            case 'o':
            case 'p':
            case 'x':
            case 'X':
              Arg.l=(Par.DateLen == e32BIT)
                    ?va_arg(Ap,long)
                    :(long)va_arg(Ap,int);     /* Half Par bei ARM 7        */
                                               /* genutzt                   */
              break;

            case 'O':
              Arg.l = va_arg(Ap,long);
              break;

            case 'q':                          /* Quad-Byte Char            */
              Arg.l = (long) va_arg(Ap,int);
              Arg.l = mNet2Host32(Arg.l);
              Par.Min= 0;
              Par.pValPos=(unsigned char*)&Arg.l;       /* pointer fuer Form ausgabe */
              break;

            case 'm':                          /* Multi-Byte Char           */
              Arg.U16 = (uint16_t) va_arg(Ap,int);
              Arg.U16 = mNet2Host16(Arg.U16);
              Par.Min= 0;
              Par.pValPos=(unsigned char*)&Arg.U16;     /* pointer fuer Form ausgabe */
              break;

            case 'c':                          /* Single-Byte Char          */
              Arg.U8 = (uint8_t) va_arg(Ap,int);
              Par.Min= 0;
              Par.pValPos=(unsigned char*)&Arg.U8;      /* pointer fuer Form ausgabe */
              break;

            case 'H':                          /* Hex items (8/16/32)bit    */
            case 'D':                          /* Decimal Items 8/16/32 bit */
            case 'B':                          /* Bitfield 8/16/32 bit      */
              Arg.pC=va_arg(Ap,char *);
              break;
          }

          switch(*fmt)
          {


            case 's':
              {
                int n,min;
                if (Arg.pC==NULL)
                {
                   Arg.pC = "(null)";
                }
                n   = strlen(Arg.pC);
                min = Par.Min;
                Par.pValPos= Arg.pU8;
                Par.Cnt = (min)                   /* auszugebende StrLen    */
                  ? (min < n) ? min : n
                  : n;                            /* strlen                 */
                Par.Min = (min < n) ? min : n ;   /* Minimale feldbreite    */
              }
              break;

            case 'd':                             /* signed int             */
            case 'i':                             /* signed int             */
              if(Arg.l<0)                         /* Negativ  ?             */
              {
                Arg.l=0-Arg.l;
                Par.Sign='-';
              }
               /*no break*/
            case 'u':
              PrimForm2 (&Par,Arg.l,10);
              break;


            case 'O':                             /* In LEN als long lesen  */
            case 'o':
              Par.Sign='\0';
              PrimForm2 (&Par,Arg.l,8);
              if (Par.Alt)
              {
                *--Par.pValPos='0';               /* octal Prefix           */
                ++Par.Cnt;
              }
              break;

            case 'X':
              Par.pForm=FormChr[UPPER_C];
              /*no break*/
            case 'x':
              if (Par.Alt)                        /* alt.   Darstellung     */
                Par.HexPref = *fmt;               /* ist activ              */
              Par.Sign='\0';
              if(Par.Max == 0)
              {
                if(Par.Min)
                  Par.Max = Par.Min;
                else
                {
                  if( Par.Zero )
                    Par.Max = 8;
                }
              }
              PrimForm2 (&Par,Arg.l,16);
              break;

            case 'p':
              if (Par.Alt)                        /* alt.   Darstellung     */
                Par.HexPref = *fmt;               /* ist activ              */
              Par.Sign='\0';

              Par.Min = Par.Max = 8;
              Par.Zero = true;
              PrimForm2 (&Par,Arg.l,16);
              break;

            case 'q':
              Par.Cnt= 4;
              break;

            case 'm':                             /* multi Char             */
              Par.Cnt= 2;
              break;

            case 'c':
              Par.Cnt= 1;
              break;

            case 'H':
              PrimMulti(&Par,fp,Arg.pU8,16,*(fmt+1));
              ++fmt;
              State=eSTART;
              continue;                           /* aussprung              */
            case 'D':
              PrimMulti(&Par,fp,Arg.pU8,10,*(fmt+1));
              ++fmt;
              State=eSTART;
              continue;                           /* aussprung              */
            case 'B':
              PrimMulti(&Par,fp,Arg.pU8,2,*(fmt+1));
              ++fmt;
              State=eSTART;
              continue;                           /* aussprung              */

            default:
              putc(*fmt,fp);                     /* ausgabe des Zeichens   */
              State=eSTART;
              continue;                           /* aussprung              */
          }
          if (Par.Min>Par.Max)
          {
             Par.Max = Par.Min;
          }
          Cnt+=ValOut(&Par,fp);                   /* formatierte ausgabe    */
          State=eSTART;
          continue;                               /* aussprung              */
                                                  /* ENDE state FORM        */
        default:
          State=eSTART;
          break;
      }
    }
  }             /* for */
return Cnt;                                       /* Zeichenzaehler         */
}

int printf(const char *format, ...)
{
	char nothing[64];
	  STRING_FILE(lclfile, nothing);
        va_list args;

        va_start( args, format );
        return xIosPrnKern( &lclfile, args, format);
}


int sprintf(char *out, const char *format, ...)
{
	  STRING_FILE(lclfile, out);
        va_list args;

        va_start( args, format );
        return xIosPrnKern( &lclfile, args, format);
}


int snprintf( char *buf, unsigned int count, const char *format, ... )
{
	  STRING_FILE(lclfile, buf);
        va_list args;

        ( void ) count;

        va_start( args, format );
        return xIosPrnKern( &lclfile, args, format);
}

int vsnprintf( char *buf, unsigned int count, const char *format, va_list args)
{
	  STRING_FILE(lclfile, buf);
    ( void ) count;

  return xIosPrnKern( &lclfile, args, format);
}


#define ALIAS(f) __attribute__ ((weak, alias (#f)))

#define DUMP_LINE_LENGTH 16
#define DUMP_MAX_SIZE 4000
#define DEBUG_PRINT_LINE_LENGTH 128

struct debugData {
	int initDone;
	uint32_t debug_level;
	uint32_t uartPort;
};

static struct debugData debugSettings = {
		0,  0, 0
};

void debug_init(uint32_t uartPort)
{
	if(!debugSettings.initDone)
	{
		debugSettings.debug_level = DEBUG_LEVEL_TO_USE;
		debugSettings.uartPort = uartPort;
		UARTInit(debugSettings.uartPort,115200);
		debugSettings.initDone = 1;
	}
}

void debug_level_set(uint32_t level)
{
	debugSettings.debug_level = level;
}

uint32_t debug_level_get(void)
{
	return debugSettings.debug_level;
}
void printInit(uint32_t uartPort) ALIAS(debug_init);
void printToUart(const char * __restrict format, ...);

static void printRoutine(const char * __restrict format, va_list arg)
{
	char debug_buffer[DEBUG_PRINT_LINE_LENGTH+1];
	int count = vsnprintf(debug_buffer,DEBUG_PRINT_LINE_LENGTH, format, arg);
	if (count>DEBUG_PRINT_LINE_LENGTH)
	{
		printToUart("PRINT mem error!");
		for(;;);
	}
    debug_buffer[DEBUG_PRINT_LINE_LENGTH]=0x0;
    UARTSend(debugSettings.uartPort, (uint8_t *)debug_buffer , strlen(debug_buffer) );
}

void printToUart(const char * __restrict format, ...)
{
    va_list arg;
    va_start(arg, format);
    printRoutine(format, arg);
    va_end(arg);
}

void debug_print(int level,const char * __restrict format, ...)
{
	if(level & debugSettings.debug_level)
	{
        va_list arg;

#ifdef DEBUG_WITH_TIMESTAMPS
        portTickType ticks = xTaskGetTickCountFromISR();
        portTickType s = ticks / 1000;
        portTickType ms = ticks - (s*1000);

        printToUart("%05d.%03d ",s,ms);
#endif
        va_start(arg, format);
        printRoutine(format, arg);
        va_end(arg);
	}
}

/* dumpes from mem to mem+size */
void dump (const int dumpLevel, constData_t mem, length_t size, const char const * message)
{
	if(dumpLevel & debugSettings.debug_level)
	{
		content_t line[DUMP_LINE_LENGTH+1];
		content_t content[((DUMP_LINE_LENGTH)*3)+1];
		int i = 0x00;

		line[DUMP_LINE_LENGTH] = 0x00;

		printToUart("This is dump  %d(0x%x) bytes from 0x%x '%s'\r\n",size,size,mem,message);

		if(size<DUMP_MAX_SIZE){
			constData_t lineStart=line; // will be changed by loop init...
			constData_t previousLine=line;
			int lineNumber=0;
			int printLine=1;
			for(;size;size--) {
				unsigned char mychar;
				unsigned char byteBuffer[4];
				if(i==0){
					previousLine = lineStart;
					lineStart=mem;
					content[0]=0x0;
				}

				mychar = *mem++;

				/* is ascii? */
				if( ((mychar)<=0x7f) && ((mychar)>=0x20)){
					if(mychar == 0x25)
					{
						/* convert the char % to * to be printable */
						line[i] = 0x2a; /***/
					}
					else
					{
						line[i] = mychar;
					}
				} else {
					/* convert the char to a dot to be printable */
					line[i] = 0x2e; /*.*/
				}

				sprintf((char*)byteBuffer, "%02x ",mychar);
				strcat((char*)content,(char*)byteBuffer);

				i++;

				if(i>(DUMP_LINE_LENGTH-1)){
					static int printDots=1;
					if(!memcmp(previousLine,lineStart,DUMP_LINE_LENGTH))
					{
						if(lineNumber)
						{
							printLine=0;
						}
					}
					else
					{
						printLine=1;
					}

					if(printLine)
					{
						printToUart(" 0x%x : %s: %s\r\n", lineStart, content, line);
						printDots=1;
					}
					else
					{
						if(printDots)
						{
							printToUart("    ....\r\n");
							printDots = 0;
						}
					}
					i = 0;
					lineNumber += 1;
				}
			}

			if(i){
				line[i] = 0x0;
				for(;i<DUMP_LINE_LENGTH;i+=1)
				{
					strcat((char*)content,"   ");
				}
				printToUart(" 0x%x : %s: %s\r\n", lineStart, content, line);
			}
			printToUart("\r\n");
		}
		else
		{
			printToUart("negative!!\r\n");
		}
	}
}

/* dumpes only icmp frames and prints the sequence number */
void dumpIcmpFrame(const int dumpLevel, data_t frame, length_t length, char * message)
{
	if(frame && length)
	{
		if(frame[0xe] == 0x45 && frame[0x17] == 0x01)
		{
		uint16_t sequence_number = frame[0x28]|frame[0x29];
#define LOCAL_BUFFER_SIZE 64
			char buffer[LOCAL_BUFFER_SIZE];
			snprintf(buffer,LOCAL_BUFFER_SIZE,"%s sequence number %d (0x%x)",message,sequence_number,sequence_number);
			dump(dumpLevel,frame,length,buffer);
		}
	}
}

