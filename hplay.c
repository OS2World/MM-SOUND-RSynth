#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#define INCL_DOSPROCESS
#define INCL_DOSSEMAPHORES
#include <os2.h>
#define  INCL_OS2MM
#include <os2me.h>
#include "proto.h"
#include "getargs.h"
#include "hplay.h"
#include "limits.h"

#define SEMWAIT_OPERATION                   9
#define SEMPOST_OPERATION                  10

void PlayThread(void *);

/* PlayList Entry */

typedef struct _PLE {
    ULONG operation;
    ULONG operand1;
    ULONG operand2;
    ULONG operand3;
}PLE;

long samp_rate  = 8000;
long bits = 16;
int quiet = FALSE;
float ampfactor = 1.0f;
ULONG volume = 100;
ULONG deviceID = 0;

/* for multimedia playlist mechanism, thread and semaphore stuff */
static unsigned long dataready = 0,
                     dataplayed = 0,
                     resetcount = 0,
                     tid = 0;

/* multimedia variables */
MCI_PLAY_PARMS mpp;
MCI_GENERIC_PARMS mgp;
MCI_WAVE_SET_PARMS wsp;
ULONG rc;
MCI_OPEN_PARMS mop;
PLE playlist[4];

/* to know if we want to end or it's just that we have been grabbed off the
   sound card to start retry */
BOOL ending = FALSE;

void mci_err(ULONG rc)
{
    const rsize = 128;
    char rbuff[rsize];

    ULONG rc2 = mciGetErrorString(rc,      // error code
                                  rbuff,   // return buffer
                                  rsize);  // rbuff size

    if (rc2 == MCIERR_SUCCESS)
        fprintf(stderr,"MCI error: %s\n",rbuff);
    else
        fprintf(stderr,"error # %d has occured!\n", rc);

    fprintf(stderr,"Switching to quiet mode\n");
    quiet = TRUE;
}

int
audio_init(int argc, char **argv)
{
 int rate = 8;

 argc = getargs("Audio Initialization", argc, argv,
                "r", "%d", &rate,   "Sample Rate in kHz - 8, 11, 22, or 44",
                "Q", NULL, &quiet,   "Quiet - No sound ouput",
                "b", "%d", &bits, "8 or 16 bit sample playback",
                "V", "%u", &volume, "Volume in %",
                "a", "%f", &ampfactor, "Amplification factor",
                "D", "%u", &deviceID, "Sound Card Device ID (0 = default device)",
                NULL);
 if (help_only || quiet)
  return (argc);
 
 switch(rate)
    {
     case  8 : samp_rate =  8000;
               break;
     case 11 : samp_rate = 11025;
               break;
     case 22 : samp_rate = 22050;
               break;
     case 44 : samp_rate = 44100;
               break;
     default : samp_rate =  8000;
    }

  if (bits != 16)
     bits = 8;

  if(!dataready) DosCreateEventSem(NULL,&dataready,0,0);
  if(!dataplayed) DosCreateEventSem(NULL,&dataplayed,0,0);

  playlist[0].operation = SEMWAIT_OPERATION;
  playlist[0].operand1  = dataready;
  playlist[0].operand2  = -1;
  playlist[0].operand3  = TRUE;

  playlist[1].operation = DATA_OPERATION;
  playlist[1].operand1  = (long) NULL;
  playlist[1].operand2  = 0;
  playlist[1].operand3  = 0;

  playlist[2].operation = SEMPOST_OPERATION;
  playlist[2].operand1  = dataplayed;

  playlist[3].operation = BRANCH_OPERATION;
  playlist[3].operand2  = 0;  // goto playlist[0]

  mop.hwndCallback   = 0;
  mop.usDeviceID     = deviceID;
  mop.pszDeviceType  = (PSZ) MAKEULONG(MCI_DEVTYPE_WAVEFORM_AUDIO, deviceID);
  mop.pszElementName = (void *) playlist;

  rc = mciSendCommand(0,
                      MCI_OPEN,                        // open message
                      MCI_WAIT | MCI_OPEN_SHAREABLE |  // message flags
                      MCI_OPEN_PLAYLIST | MCI_OPEN_TYPE_ID,
                      &mop,                            // parameters
                      0);
  if (rc != MCIERR_SUCCESS) mci_err(rc);

  // set device parameters

  if(volume > 100) volume = 100;

  wsp.ulSamplesPerSec = samp_rate;
  wsp.usBitsPerSample = bits;
  wsp.usChannels = 1;
  wsp.ulLevel = volume;
  wsp.ulAudio = MCI_SET_AUDIO_ALL;

  if (!quiet)
  {
     wsp.hwndCallback = 0;
     rc = mciSendCommand(mop.usDeviceID,
                         MCI_SET,
                         MCI_WAIT |
                         MCI_WAVE_SET_SAMPLESPERSEC |
                         MCI_WAVE_SET_BITSPERSAMPLE |
                         MCI_WAVE_SET_CHANNELS,
                         &wsp,
                         0);
     if (rc != MCIERR_SUCCESS) mci_err(rc);
  }

  if (!quiet)
  {
     mpp.hwndCallback = 0;
     rc = mciSendCommand(mop.usDeviceID,
                         MCI_SET,
                         MCI_WAIT | MCI_SET_AUDIO |
                         MCI_SET_VOLUME,
                         &wsp,
                         0);
     if (rc != MCIERR_SUCCESS) mci_err(rc);
  }

  if(!quiet) tid = _beginthread(PlayThread,0,8096,0);

  _control87(EM_INVALID | EM_DENORMAL | EM_ZERODIVIDE | EM_OVERFLOW | EM_UNDERFLOW | EM_INEXACT, MCW_EM);

  return (argc);
}

/* arg, I have to start a thread just to _wait_ for the end of the
   playback, geez... - Samuel */

void PlayThread(void *arg)
{
   DosPostEventSem(dataplayed);

   mpp.hwndCallback = 0;
   rc = mciSendCommand(mop.usDeviceID,
                       MCI_PLAY,
                       MCI_WAIT,
                       &mpp,
                       0);
   if (rc != MCIERR_SUCCESS) mci_err(rc);

   if(!ending)
      while(DosWaitEventSem(dataplayed, 5000))
      {
         audio_init(0,NULL);
         quiet = FALSE;
      }

   /* semaphore will be received by another PlayThread
      when the device gets back to us - Samuel */
   else
      DosPostEventSem(dataplayed); /* for audio_term() */
}


void
audio_term(void)
{

  if (quiet) return;

  ending = TRUE;

  // close device
  if(DosWaitEventSem(dataplayed, -1)) return;
  DosResetEventSem(dataplayed,&resetcount);

  free((void*) playlist[1].operand1);
  playlist[1].operation = EXIT_OPERATION;

  DosPostEventSem(dataready);

  if(DosWaitEventSem(dataplayed, -1)) return;
  DosResetEventSem(dataplayed,&resetcount);

  DosCloseEventSem(dataready); dataready = 0;
  DosCloseEventSem(dataplayed); dataplayed = 0;

  mgp.hwndCallback = 0;
  rc = mciSendCommand(mop.usDeviceID,
                      MCI_STOP,
                      MCI_WAIT,
                      &mgp,
                      0);
  if (rc != MCIERR_SUCCESS) mci_err(rc);

  mgp.hwndCallback = 0;
  rc = mciSendCommand(mop.usDeviceID,
                      MCI_CLOSE,
                      MCI_WAIT,
                      &mgp,
                      0);
  if (rc != MCIERR_SUCCESS) mci_err(rc);
}

void
audio_play(int n, short *data, char *sentence, char *phone)
{
 extern int verbose, text;

 if (quiet) return;

 /* my cheezy amplifier - Samuel */

 if(ampfactor != 1.0f)
 {
    short *current = data,
          *end = data + n;

    while(current < end)
    {
       if (*current < 0)
       {
          if ((*current * ampfactor) > SHRT_MIN)
             *current = (short) (*current * ampfactor);
          else
             *current = SHRT_MIN;
       }
       else if (*current > 0)
       {
          if ((*current * ampfactor) < SHRT_MAX)
             *current = (short) (*current * ampfactor);
          else
             *current = SHRT_MAX;
       }
       current++;
    }

 }

 if(DosWaitEventSem(dataplayed, -1)) return;
 DosResetEventSem(dataplayed,&resetcount);

 if (bits == 8)
   {
     char *plabuf;

     plabuf = (char *) malloc(n * sizeof(char));
     if (plabuf)
       {
        char *p = plabuf;
        char *e = p + n;
        while (p < e)
           *p++ = (char) (*data++ / 256 + 0.5) + 128;

        free((void *) playlist[1].operand1);
        playlist[1].operand1  = (long) plabuf;
        playlist[1].operand2  = n;

        if(text) printf("%s",sentence);
        if(verbose) printf("\n%s\n",phone);
        free(sentence);
        free(phone);

        DosPostEventSem(dataready);
       }
     else
       {
         fprintf(stderr, "Insufficient memory for Play Buffer\n\n");
         return;
       }
   }
 else  /* if bits == 16 */
   {
    short *plabuf;

    plabuf = (short *) malloc(n * sizeof(short));
    if (plabuf)
       {
        short *p = plabuf;
        short *e = p + n;
        while (p < e)
           *p++ = *data++;

        free((void *)playlist[1].operand1);
        playlist[1].operand1  = (long) plabuf;
        playlist[1].operand2  = n*2; /* don't ask, don't know, but works */

        if(text) printf("%s",sentence);
        if(verbose) printf("\n%s\n",phone);
        free(sentence);
        free(phone);

        DosPostEventSem(dataready);
       }
     else
       {
         fprintf(stderr, "Insufficient memory for Play Buffer\n\n");
         return;
       }
   }
}
