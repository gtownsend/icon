#include "../h/gsupport.h"

#if AMIGA && __SASC
#include <workbench/startup.h>
#include <rexx/rxslib.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <proto/wb.h>
#include <proto/rexxsyslib.h>
#include <proto/exec.h>

int _WBargc;
char **_WBargv;
struct MsgPort *_IconPort = NULL;
char *_PortName;

/* This is an SAS/C auto-initialization routine.  It extracts the
 * filename arguments from the ArgList in the Workbench startup message
 * and generates an ANSI argv, argc from them.  These are given the
 * global pointers _WBargc and _WBargv.  It also checks the Tooltypes for
 * a WINDOW specification and points the ToolWindow to it.  (NOTE: the
 * ToolWindow is a reserved hook in the WBStartup structure which is
 * currently unused. When the Workbench supports editing the ToolWindow,
 * this ToolType will become obsolete.)  The priority is set to 400 so
 * this will run before the stdio initialization (_iob.c).  The code in
 * _iob.c sets up the default console window according to the ToolWindow
 * specification, provided it is not NULL. 
 */

int __stdargs _STI_400_WBstartup(void) {
   struct WBArg *wba;
   struct DiskObject *dob;
   int n;
   char buf[512];
   char *windowspec;

   _WBargc = 0;
   if(_WBenchMsg == NULL || Output() != NULL) return 0;
   _WBargv = (char **)malloc((_WBenchMsg->sm_NumArgs + 4)*sizeof(char *));
   if(_WBargv == NULL) return 1;
   wba = _WBenchMsg->sm_ArgList;

   /* Change to the WB icon's directory */
   CurrentDir((wba+1)->wa_Lock);

   /* Get the window specification */
   if(dob = GetDiskObject((wba+1)->wa_Name)) {
      if(dob->do_ToolTypes){
         windowspec = FindToolType(dob->do_ToolTypes, "WINDOW");
         if (windowspec){
            _WBenchMsg->sm_ToolWindow = malloc(strlen(windowspec)+1);
            strcpy(_WBenchMsg->sm_ToolWindow, windowspec);
            }
         }
      FreeDiskObject(dob);
      }

   /* Create argc and argv */
   for(n = 0; n < _WBenchMsg->sm_NumArgs; n++, wba++){
      if (wba->wa_Name != NULL &&
              NameFromLock(wba->wa_Lock, buf, sizeof(buf)) != 0) {
         AddPart(buf, wba->wa_Name, sizeof(buf));
         _WBargv[_WBargc] = (char *)malloc(strlen(buf) + 1);
         if (_WBargv[_WBargc] == NULL) return 1; 
         strcpy(_WBargv[_WBargc], buf);
         _WBargc++;
         }
      }

   /* Just in case ANSI is watching ... */
   _WBargv[_WBargc] = NULL;
   }

/* We open and close our message port with this auto-initializer and
 * auto-terminator to minimize disruption of the Icon code.
 */

void _STI_10000_OpenPort(void) {
   char  *name;
   char  *end;
   int   n = 1;
   char  buf[256];

   if( GetProgramName(buf, 256) == 0) {
     if (_WBargv == NULL) return; 
     else strcpy(buf, _WBargv[0]);
     }

   name = FilePart(buf);
   _PortName = malloc(strlen(name) + 2);
   strcpy(_PortName, name);
   end = _PortName + strlen(_PortName);
   /* In case there are many of us */ 
   while ( FindPort(_PortName) != NULL ) {
      sprintf(end, "%d", n++);
      if (n > 9) return;
      }
   _IconPort = CreatePort(_PortName, 0);
   }

void _STD_10000_ClosePort(void) {
   struct Message *msg;

   if (_IconPort) {
      while (msg = GetMsg(_IconPort)) ReplyMsg(msg);
      DeletePort(_IconPort);
      }
   }

/*
 * This posts an error message to the ARexx Clip List.
 * The clip is named <_PortName>Clip.<errorcount>.  The value
 * string contains the file name, line number, error number and
 *  error message.
 */
static int errorcount = 0;

void PostClip(char *file, int line, int number, char *text) {
   struct MsgPort *rexxport;
   struct RexxMsg *rxmsg;
   char name[128];
   char value[512];

   if ( _IconPort ) {
      if ( rxmsg = CreateRexxMsg(_IconPort, NULL, NULL) ) {
         errorcount++;
         sprintf(name, "%sClip.%d", _PortName, errorcount);
         sprintf(value, "File: %s Line: %d Number: %d Text: %s",
                         file, line, number, text);
         rxmsg->rm_Action = RXADDCON;
         ARG0(rxmsg) = name;
         ARG1(rxmsg) = value;
         ARG2(rxmsg) = (unsigned char *)(strlen(value) + 1);
         Forbid();
         rexxport = FindPort("REXX");
         if ( rexxport ) { 
            PutMsg(rexxport, (struct Message *)rxmsg);
            WaitPort(_IconPort);
            }
         Permit();
         GetMsg(_IconPort);
         DeleteRexxMsg(rxmsg);
         }
      }
   }


/*
 * This function sends a message to the resident ARexx process telling it to
 * run the specified script with argument a stem for the names of the clips
 * containing error information.  The intended use is to invoke an editor
 *  when a fatal error is encountered.
 */

void CallARexx(char *script) {
   struct MsgPort *rexxport;
   struct RexxMsg *rxmsg;
   char command[512];

   if ( _IconPort ) {
      if ( rxmsg = CreateRexxMsg(_IconPort, NULL, NULL) ) {
         sprintf(command, "%s %sClip", script, _PortName);
         rxmsg->rm_Action = RXCOMM | RXFB_NOIO;
         ARG0(rxmsg) = command;
         if (FillRexxMsg(rxmsg,1,0) ) {
            Forbid();
            rexxport = FindPort("REXX");
            if ( rexxport ) { 
               PutMsg(rexxport, (struct Message *)rxmsg);
               WaitPort(_IconPort);
               }
            Permit();
            GetMsg(_IconPort);
            ClearRexxMsg(rxmsg,1);
            }
         DeleteRexxMsg(rxmsg);
         }
      }
   }


#else                                  /* AMIGA && __SASC */

char junkclocal; /* avoid empty module */

#endif					/* AMIGA && __SASC */
