SIP Protocol
------------

CAUTION: THIS IS A ALPHA STAGE PLUGIN. IT CAN DO VERY BAD THINGS. USE AT YOUR OWN RISK.

This is a SIP/SIMPLE protocol based on pjsip lib.

Currently it allows to make/receive calls and im messages. All the GUI is handled by Voice Service plugin. 

WARNING: You can create only one instance of the protocol. If you create the second one miranda will crash and keep crashing at each startup. I'll fix it soon


Known bugs:
- When you login, you will only show online the contacts that already are online (contacts that came online after that will stay offline). Only happens with some servers.
- Password is sent as plain text


Todo:
- More than one instance
- Protocol icons
- Use netlib to send/receive packages?
- Ask for new password at first login if "Save password" is not checked
- User search
- Proxy support (not sip proxy, but socks proxy)


To report bugs/make suggestions, go to the forum thread: http://forums.miranda-im.org/showthread.php?t=23655