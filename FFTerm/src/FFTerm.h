#ifndef _FFTERM_H_
#define _FFTERM_H_

#include "FFTerm-Types.h"
#include "FFTerm-Error.h"

#define	FFT_ENABLE_ECHO_INPUT	0x0004
#define FFT_ENABLE_LINE_INPUT	0x0002
#define FFT_ENABLE_WINDOWS		0x8000

#define FFT_MODE_DEFAULT		0
#define WINDOWS

#define FFT_MAX_CMD_NAME		10
#define FFT_MAX_CMD_PROMPT		20

#define FFT_MAX_CMD_LINE_INPUT	255
#define FFT_MAX_CMD_LINE_ARGS	10

#define FFT_RETURN				0x0A
#define FFT_BACKSPACE			0x08
#define FFT_CRLF				"\n"

//#define FFT_GETCH(X)


#define FFT_KILL_CONSOLE		-666	///< Special return value from any Command to kill the console.

typedef int					 (*FFT_FN_COMMAND)	(int argc, char **argv);
typedef const FF_T_INT8		*(*FFT_FN_GETERRSTR)(FF_T_SINT32 pa_iErrorCode);

typedef struct _FFT_COMMAND {
	FF_T_INT8		cmdName[FFT_MAX_CMD_NAME];
	FFT_FN_COMMAND	fnCmd;
	struct _FFT_COMMAND	*pNextCmd;		///< Pointer to the next command in the linked list.
} FFT_COMMAND;


typedef struct _FF_CONSOLE {
				FF_T_INT8	strCmdPrompt[FFT_MAX_CMD_PROMPT];
				FF_T_INT8	strCmdLine[FFT_MAX_CMD_LINE_INPUT];
				FF_T_INT8	*pArgs[FFT_MAX_CMD_LINE_ARGS];
				FF_T_UINT32	Mode;
	
	volatile	FF_T_BOOL	bKill;		///< Volatile because it can be set in a multi-threaded environment. (Or the exit command!);
				FFT_COMMAND	*pCommands;
				FILE		*pStdIn;
				FILE		*pStdOut;
} FFT_CONSOLE;

FFT_CONSOLE *FFTerm_CreateConsole(FF_T_INT8 *pa_strCmdPrompt, FILE *pa_pStdIn, FILE * pa_pStdOut, FF_T_SINT32 *pError);
FF_T_SINT32 FFTerm_StartConsole(FFT_CONSOLE *pConsole);
FF_T_SINT32	FFTerm_AddCmd(FFT_CONSOLE *pConsole, const FF_T_INT8 *pa_cmdName, FFT_FN_COMMAND pa_fnCmd);
FF_T_SINT32 FFTerm_RemoveCmd(FFT_CONSOLE *pConsole, const FF_T_INT8 *pa_cmdName);
FF_T_SINT32 FFTerm_SetConsoleMode(FFT_CONSOLE *pConsole, FF_T_UINT32 Mode);
FF_T_SINT32 FFTerm_GetConsoleMode(FFT_CONSOLE *pConsole, FF_T_UINT32 *Mode);


#endif
