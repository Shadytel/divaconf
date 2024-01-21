#include <stdio.h>
#ifdef WIN32
    #include <conio.h>
#endif
#include "dssdk.h"
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h> // Surprisingly, Windows won't pitch a fit about this.
#include <string.h>


/*
 * Some globals
 */
DivaAppHandle   hApp = NULL;
DivaCallHandle  hSdkCall[24];
AppCallHandle   hMyCall[24];
AppCallHandle   ConfHandle;
DivaCallHandle  ConfCallhd = NULL;
// DivaConferenceInfo confinfo;

unsigned short maxannc = 0;
char filepath[80] = {0};
unsigned char participants = 0; // Number of people in the conference
char result3; // Variable for returning result codes
char destination[32] = {0};
BOOL joinstate = TRUE;
BOOL reject[24] = {FALSE};

#ifdef WIN32
    struct _stat sts;
#else
    struct stat sts;
#endif

// The format for call properties is kinda annoying. Is there some part of the C
// standard I'm missing to only have it on the argument line? The demo code all
// calls DivaSetCallProperties() with a pointer to an actual int.
int tail = 128;
const int predelay = 0;

int getchannum( PVOID callhandle )
{
    int channelnum = 1;
    while ( channelnum < 24 ) {
        if ( hMyCall[channelnum] == callhandle ) break;
        channelnum++;
    }
    if ( channelnum > 24 ) {
        printf( "ERROR: Could not find channel for event. Default identifier assigned.\n" );
        return 0;
    }
    else return channelnum;
}
char play( DivaCallHandle playdev, char * filename, DivaAudioFormat playbackformat, DWORD offset ) {

     DivaVoiceDescriptor playbackdescriptor[1];

     playbackdescriptor[0].Size = sizeof(DivaVoiceDescriptor);
     playbackdescriptor[0].StartOffset = offset;
     playbackdescriptor[0].Duration = 0;
     playbackdescriptor[0].DataFormat = playbackformat;
     playbackdescriptor[0].DataSource = DivaVoiceDataSourceFile;
     playbackdescriptor[0].PositionFormat = DivaVoicePositionFormatBytes;
     playbackdescriptor[0].Source.File.pFilename = filename;

  return(DivaSendVoiceEx( playdev, 1, playbackdescriptor, FALSE, 0 ) );

}

void confprops() {
    DivaConferenceInfo conferenceinfo;
    conferenceinfo.Size = sizeof(DivaConferenceInfo); // For reasons, the API wants this filled out before it'll actually send conference info
    DivaGetConferenceInfo( ConfCallhd, &conferenceinfo );
    //printf("DEBUG: Diva says there's %d callers\n", conferenceinfo.CurrentMembers);
    switch(conferenceinfo.State) {
        case DivaConferenceStateAdding:
            //printf("DEBUG: Conference state is adding.\n");
            break;
        case DivaConferenceStateIdle:
            //printf("DEBUG: Conference state is idle.\n");
        case DivaConferenceStateConnected:
            if (joinstate == TRUE) {
                if (participants == 1) {
                    if (maxannc > 0) {
                        // Supposedly, rand() should Just Work in Windows.
                        snprintf(filepath, 79, "confhold/%d.pcm", rand() % maxannc);
                        play(ConfCallhd, filepath, DivaAudioFormat_Raw_uLaw8K8BitMono, 0);
                    }
                    else DivaSendVoiceFile( ConfCallhd, "join.wav", FALSE);
                }
                else DivaSendVoiceFile( ConfCallhd, "join.wav", FALSE);
            }
            else DivaSendVoiceFile( ConfCallhd, "drop.wav", FALSE);
            break;
        case DivaConferenceStateRemoving:
            //printf("DEBUG: Conference state is removing\n");
            break;
        default:
            printf("conferenceinfo.State value is not recognized: %d\n", conferenceinfo.State);
    }
    return;
}

void CallbackHandler ( DivaAppHandle hApp, DivaEvent Event, PVOID Param1, PVOID Param2 )
{
    DivaCallInfo callinfo;
    callinfo.Size = sizeof( DivaCallInfo );
    DWORD channum;

    switch (Event)
    {
    case DivaEventIncomingCall:
        DivaGetCallInfo( Param1, &callinfo );
        channum = callinfo.AssignedBChannel;
        //printf("DEBUG: Called party number is %s\n", callinfo.CalledNumber);
        hSdkCall[channum] = Param1;
        if (destination[0] != 0x00) {
            if (strcmp(destination, callinfo.CalledNumber) == 0) {
                reject[channum] = FALSE;
                printf("Incoming call to bridge on channel %d from phone number: %s\n", callinfo.AssignedBChannel, callinfo.CallingNumber);
                DivaAnswer ( hSdkCall[channum], hMyCall[channum], DivaCallTypeVoice );
            }
            else {
                reject[channum] = TRUE;
                DivaReject(hSdkCall[channum], TRUE);
            }
            break;
        }
        printf("Incoming call to bridge on channel %d from phone number: %s\n", callinfo.AssignedBChannel, callinfo.CallingNumber);
        DivaAnswer ( hSdkCall[channum], hMyCall[channum], DivaCallTypeVoice );
        break;

    case DivaEventCallConnected:
        if ( participants == 0 ) {
           result3 = (DivaCreateConference( hApp, 0, ConfHandle, &ConfCallhd));
           if ( result3 != 0 ) {// In theory, some error handling should go here
               printf("ERROR: Conference creation failed with result %d!\n", result3);
           }
        }
        else if (participants == 1) {
            // We're about to start the conf, so dispense with the MOH and get ready
            // to play the join tone.

            // TO DO: While the Diva hardware is forgiving, this doesn't have enough
            // time to execute and a DivaEventSendVoiceCanceled is generated.
            DivaStopSending(ConfCallhd);
        }

        channum = getchannum( Param1 );
        if (tail != 0) {
            DivaSetCallProperties( hSdkCall[channum], DivaCPT_EchoCancellerTailLength,
                                   (DivaCallPropertyValue *) &tail, sizeof( int ) );
            DivaSetCallProperties( hSdkCall[channum], DivaCPT_EchoCancellerPreDelay,
                                   (DivaCallPropertyValue *) &predelay, sizeof( int ) );
            if (DivaEnableEchoCanceller( hSdkCall[channum], TRUE ) != DivaSuccess)
                printf("WARNING: Echo canceller activation failed! Continuing without...\n");
        }

        /* joinstate is used to avoid a slight annoyance of the hardware; when
         * adding a new caller to the conference, any play commands issued, while
         * they'll complete successfully from a software perspective, will actually
         * encounter silence during the first second or so of playback. By waiting for
         * this process to actually complete first, we can sidestep that; the card sends
         * another event when it's finished with that task (DivaEventConferenceInfo) and
         * we deal with it there.
         */
        joinstate = TRUE;

        // Yes, yes, the API keeps count of participants, but, uhm, we're doing it too.
        // Why exactly? HISSSSSSSS PROPRIETARY REASONINGSES!!!
        participants++;
        DivaAddToConference( ConfCallhd, hSdkCall[channum] );
        return;
        

    case DivaEventConferenceInfo:
        // EventSpecific1 (Param1?) contains the info in question. Consider using that
        // as an argument instead of assuming the global handle.
        confprops();
        break;

    case DivaEventSendVoiceDone:
        // This is an obsolete event and should be ignored.
        break;
        
    case DivaEventSendVoiceCanceled:
        printf("File playback event canceled\n");
        break;
    case DivaEventSendVoiceEnded:
        // We don't need any handling code at the moment other than for the music on hold mode.
        if (participants == 1) {
            if (maxannc > 0) {
                snprintf(filepath, 79, "confhold/%d.pcm", rand() % maxannc);
                play(ConfCallhd, filepath, DivaAudioFormat_Raw_uLaw8K8BitMono, 0);
            }
        }
        break;

    case DivaEventDTMFReceived:
        // Nope.
        break;

    case DivaEventCallInfo:
         //printf("DEBUG: Grabbing call info - Param1 is %ld, Param2 is %ld\n", (long) Param1, (long) Param2);
         channum = getchannum( Param1 );
       	 DivaGetCallInfo( hSdkCall[channum], &callinfo );
         if (callinfo.CallState == 8) joinstate = FALSE;
         else if (callinfo.CallState == 6) return; // We already say a call is coming in, no need to put it twice
         else printf( "Call state is returned back as %d\n", callinfo.CallState);
         return;

    case DivaEventCallDisconnected:
        DivaGetCallInfo( Param2, &callinfo );
        channum = callinfo.AssignedBChannel;
        if (reject[channum] == TRUE)
            return; // Don't run the disconnect routine for callers who were rejected. That, uhh, kinda pisses the card off.
        printf( "Caller on channel %d disconnected from conference.\n", channum );
        joinstate = FALSE; // We generally don't hit this to indicate a call disconnect, but just in case...
        DivaRemoveFromConference( ConfCallhd, Param2 );
        hSdkCall[channum] = 0;
        DivaCloseCall ( Param2 );
        participants--;
        if ( participants == 0 ) {
            DivaDestroyConference( ConfCallhd, TRUE );
            printf( "Conference empty. Cleaning up...\n" );
        }
        return;

    case DivaEventDataAvailable:
        break;

    case DivaEventCallProgress:
        //printf( "Call progress event received.\n");
        break;

    default:
        printf( "Unhandled event received: Event: %u, Param1: %ld, Param2: %ld\n", Event, (long) Param1, (long) Param2);
        break;
    }
}


int main(int argc, char* argv[])
{
    // Maybe we should actually test this on Windows...
#ifdef WIN32
    srand(time(NULL));
#else
    srandom(time(NULL));
#endif
    int c;
    char channelid = 1;

    if (DivaInitialize() != DivaSuccess) {
        printf("ERROR: Diva API couldn't initialize!\n");
        return -1;
    }
    ConfHandle = (void *) 0x11223344;
    while ( channelid < 24 )
    {
    hMyCall[channelid] = (void *) ( 0x11223344 + channelid );
    channelid++;
    }

    /* snprintf is completely unnecessary here since we control the input and even with
     * max integer size, there's no way we're overflowing the array. To satisfy the
     * kneejerk ZOMG NO SPRINTF turds though, well, we're snprintf'ing it up.
     */

    snprintf(filepath, 79, "confhold/0.pcm");
#ifdef WIN32
    while (_stat(filepath, &sts) != -1) {
#else
    while (stat(filepath, &sts) != -1) {
#endif
        maxannc++;
        snprintf(filepath, 79, "confhold/%d.pcm", maxannc);
    }
    if ( DivaRegister ( &hApp, DivaEventModeCallback, 
                        (void *) CallbackHandler, 0, 23, 7, 2048 ) != DivaSuccess )
    {
        printf("ERROR: DivaRegister failed! Is your Diva card working?\n");
        DivaTerminate();
        return -1;
    }

    // TO DO: The argument acceptance code is, uhh, kinda sloppy. Maybe it shouldn't be.

    // If no arguments specified, listen on all spans for any phone number.
    if (argc == 1) {

        if ( DivaListen ( hApp, DivaListenAllVoice, LINEDEV_ALL, NULL ) != DivaSuccess ) {
            printf("ERROR: DivaListen failed! Is your Diva card working?\n");
            DivaUnregister(hApp);
            DivaTerminate();
            return -1;
        }

    }

    // For just one argument, specify the line device or zero for all.
    else if (argc == 2) {
        if ( DivaListen ( hApp, DivaListenAllVoice, atoi(argv[1]), NULL ) != DivaSuccess ) {
            printf("ERROR: DivaListen failed! Is your Diva card working?\n");
            DivaUnregister(hApp);
            DivaTerminate();
            return -1;
        }
    }

    else if (argc == 3) {
        if ( DivaListen ( hApp, DivaListenAllVoice, atoi(argv[1]), NULL ) != DivaSuccess ) {
            printf("ERROR: DivaListen failed! Is your Diva card working?\n");
            DivaUnregister(hApp);
            DivaTerminate();
            return -1;
        }
        tail = atoi(argv[2]);
        if (tail > 256)
            printf("WARNING: Echo canceller tail length over 256. Please change accordingly.\n");
    }

    // For two arguments, specify the line device and called party number to run the conference on.
    else if (argc > 3) {
        tail = atoi(argv[2]);
        if (tail > 256)
            printf("WARNING: Echo canceller tail length over 256. Please change accordingly.\n");
        strncpy(destination, argv[3], 31);
        if ( DivaListen ( hApp, DivaListenAllVoice, atoi(argv[1]), destination ) != DivaSuccess ) {
            printf("ERROR: DivaListen failed! Is your Diva card working?\n");
            DivaUnregister(hApp);
            DivaTerminate();
            return -1;
        }
    }

    printf ( "Conference program running, press <q> to terminate\n" );

    while ( 1 )
    {
#ifdef WIN32
        c = _getch ( );
#else
        c = getc (stdin);
#endif
        if ( c == 'q' )
            break;
    }
    printf ( "Conference stopped\n" );

    DivaUnregister ( hApp );
    DivaTerminate ();

    return ( 0 );
};
