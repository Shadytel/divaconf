# divaconf
Conference application for Sangoma Diva cards
      
This is a simple conferencing application for the Diva family of telephony cards that show up from time to time on eBay. While we've tested this with V-PRI, Server PRI and BRI cards, the API is abstracted enough from the telephony interface that it'll likely Just Work with the analog cards as well. While this has been tested extensively on Linux, it'll likely work as is in a Windows environment.

 Usage
=======

If you're just bored, don't want to read any further and just want to run a conference, just type 'make' and run the divaconf executable. With no arguments, the software will listen on all available ports and phone numbers with the default level of echo cancellation.

The first argument is the span/port number to listen on. An argument of 0 specifies all interfaces, while any single port can also be singled out. For example: ./divaconf 2 will only listen on the second port.

The second argument configures the echo cancellation routine on the cards. First off, the routine is impressive - even against the harshest of line conditions it tends to hold up better than most echo cancellers. The tradeoff to this though is the routine incurs latency, which depending on your environment, might be even worse than echo. If that's you, set this argument to 0 and move on. The default tail length in the routine is 128 milliseconds, and for some cards, that's the maximum possible value - check any documentation you might have. A good number of cards can extend the tail to 256 milliseconds if necessary.

The third argument is the called party number. If you're running multiple applications on the same port, a directory number can be specified to the conference software to stop it from answering calls destined for other things. Just give it any string the Diva can accept as a destination. While ISDN networks can technically support all sorts of strange characters in a called party number information element, if you're a buzzkill like some of these developers are, it's only supposed to be numbers or something. So just be ready for the card to grumble if you try and feed it a * or # in the IE.

So for example, this'll launch the conference application only on the first port with a 256 millisecond echo canceller tail, and only accepting calls on the number 31337:

./divaconf 1 256 31337

Pretty simple, right? All of these are optional, by the way.

Finally, there's the confhold directory. When there's only one call left on the bridge, it'll start playing randomized music on hold from this directory. We've included some sample tracks - just remove them if you'd like it to be silent. If you'd like to add your own, this is all headerless mu-law sampled at 8 khz. Just name the tracks in ascending order with a .pcm extension (0.pcm, 1.pcm, 2.pcm, etc), and the program will automatically mark it for playback when it starts.

 To Do
========

Let us know if you like this bridge! Most of the time, a phone conf is a pretty laid back environment, and depending on your choice in friends, no moderation should be necessary. There's some cases when muting/kicking/locking is just a fact of life though. For this, we plan to at some point make an elaborate curses-style UI that lets you do all these functions, and maybe more importantly, dial calls and pull them into the bridge. If this sort of addition would make your life easier, let us know to prioritize it.
