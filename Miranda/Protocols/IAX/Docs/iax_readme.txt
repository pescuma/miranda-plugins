IAX protocol
------------

CAUTION: THIS IS A BETA STAGE PLUGIN. IT CAN DO VERY BAD THINGS. USE AT YOUR OWN RISK.

This is a protocol that make voice calls using Inter-Asterisk eXchange (IAX) protocol. It is based in the IAXCLient library and in the work of sje in his IAX plugin.

Currently it allows to make and receive calls. All the GUI is handled by Voice Service plugin. It has no contacts (nor it will have). It also allows at most 3 concurrent calls. If someone needs more than that just tell me.

I've created this protocol with 2 goals: have an IAX implementation using the new protocol API and improve voice service.


Todo:
- Use netlib to send/receive packages?
- Ask for new password at first login if "Save password" is not checked


To report bugs/make suggestions, go to the forum thread: http://forums.miranda-im.org/showthread.php?p=195413